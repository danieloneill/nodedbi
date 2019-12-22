#include <v8.h>
#include <node.h>
#include <node_object_wrap.h>
#include <node_buffer.h>
#include <nan.h>

#include <string.h>
#include <stdlib.h>

#include "connection.h"
#include "query.h"

void DBQuery::Init( const v8::FunctionCallbackInfo<v8::Value>& args )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();

	// Prepare constructor template
	v8::Local<v8::FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate);
	tpl->SetClassName(v8str("DBQuery"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "count", Count);
	NODE_SET_PROTOTYPE_METHOD(tpl, "fieldCount", FieldCount);
	NODE_SET_PROTOTYPE_METHOD(tpl, "fieldName", FieldName);
	NODE_SET_PROTOTYPE_METHOD(tpl, "fieldIndex", FieldIndex);
	NODE_SET_PROTOTYPE_METHOD(tpl, "seek", Seek);
	NODE_SET_PROTOTYPE_METHOD(tpl, "previousRow", PreviousRow);
	NODE_SET_PROTOTYPE_METHOD(tpl, "nextRow", NextRow);
	NODE_SET_PROTOTYPE_METHOD(tpl, "currentRow", CurrentRow);
	NODE_SET_PROTOTYPE_METHOD(tpl, "value", Value);
	NODE_SET_PROTOTYPE_METHOD(tpl, "begin", Begin);
	NODE_SET_PROTOTYPE_METHOD(tpl, "commit", Commit);
	NODE_SET_PROTOTYPE_METHOD(tpl, "rollback", Rollback);

	v8::Local<v8::Function> function = tpl->GetFunction();
	v8::Handle<v8::Object> instance = Nan::NewInstance(function).ToLocalChecked();

	this->Wrap(instance);

	args.GetReturnValue().Set(instance);
}

DBQuery::DBQuery( DBConnection *parent )
	: m_parent(parent),
	m_result(NULL)
{
	m_parent->Ref();
}

DBQuery::~DBQuery()
{
	if( m_result )
		DBI::dbi_result_free( m_result );
	m_parent->Unref();
}

#define FQINITIAL 2048
#define FQINC 64
bool DBQuery::query( const v8::FunctionCallbackInfo<v8::Value>& args )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	bool inArray = false;
	v8::Local< v8::Array > arrayObj;
	
	// Also accept: query( <format>, [<arg1>, <arg2>, ...] );
	if( args.Length() == 2 && args[1]->IsArray() )
	{
		inArray = true;
		arrayObj = v8::Local< v8::Array >::Cast( args[1] );
	}

	v8::Local<v8::String> stringArg = args[0]->ToString();
	int rawQueryLength = stringArg->Utf8Length();
	char *rawQuery = (char*)malloc( rawQueryLength + 1 );
	args[0]->ToString()->WriteUtf8( rawQuery, rawQueryLength );
	rawQuery[ rawQueryLength ] = '\0';

	ssize_t fmtCapacity = FQINITIAL;
	char *fmtQuery = (char *)malloc( fmtCapacity+1 );
	int fqPos = 0;

	unsigned int idx;
	char idxStr[6];
	int idxPos = 0;
	bool inIndex = false, atEnd = false;

	int qlen = strlen( rawQuery );
	for( int x=0; x < qlen; x++ )
	{
		char c = rawQuery[x];
		if( fqPos + 2 >= fmtCapacity ) // +2 because we may have a spare byte + terminator.
		{
			// Enlarge
			fmtCapacity += FQINC;
			fmtQuery = (char *)realloc( fmtQuery, fmtCapacity+1 );
		}

		if( !inIndex )
		{
			if( c == '%' )
			{
				idxPos = 0;
				inIndex = true;
				continue;
			}

			fmtQuery[ fqPos++ ] = c;
			fmtQuery[ fqPos ] = '\0';
			continue;
		}

		// Just an escaped % character:
		if( c == '%' )
		{
			inIndex = false;
			fmtQuery[ fqPos++ ] = '%';
			fmtQuery[ fqPos ] = '\0';
			continue;
		}
		// Still reading an index...
		else if( c >= '0' && c <= '9' && idxPos < 5 )
		{
			idxStr[idxPos++] = c;
			idxStr[idxPos] = '\0';

			// Basically if the query ends on an index, fall through so it processes it.
			// eg; SELECT * FROM table WHERE id=%1 <- query ends while scanning an index.
			if( x+1 != qlen )
				continue;
			atEnd = true;
		}

		inIndex = false;
		idxPos = 0;
		idx = atoi(idxStr); // Start at %1, actually is arg position 1 luckily.

		if( idx <= 0 || ( ( inArray && idx > arrayObj->Length() ) || ( !inArray && idx >= (unsigned int)args.Length() ) ) )
		{
			// Error.
			isolate->ThrowException(v8::Exception::TypeError( v8str("Index is invalid.") ) );
			delete rawQuery;
			return false;
		}

		v8::Local< v8::Value > val;
		if( !inArray )
			val  = args[idx];
		else
			val  = arrayObj->Get(idx-1);

		// String:
		if( val->IsString() )
		{
			// Quote it:
			v8::Local< v8::String > s = val->ToString();

			char *quoted;
			size_t bufLength = s->Utf8Length();
			char *unquoted = (char *)malloc( bufLength + 1 );
			s->WriteUtf8( unquoted, bufLength );
			unquoted[bufLength] = '\0';

			size_t len;

			// Binary or string?
			bool isBinary = false;
			for( unsigned int x=0; x < bufLength && !isBinary; x++ )
				if( unquoted[x] < 32 || unquoted[x] > 126 ) isBinary = true;

			if( isBinary )
				len = DBI::dbi_conn_quote_binary_copy( m_parent->m_conn, (const unsigned char*)unquoted, bufLength, (unsigned char**)&quoted );
			else
				len = DBI::dbi_conn_quote_string_copy( m_parent->m_conn, unquoted, &quoted );

			free( unquoted );

			if( len == 0 )
			{
				// Error.
				isolate->ThrowException(v8::Exception::TypeError( v8str("Parameter quotation failed.") ) );
				delete rawQuery;
				return false;
			}
			
			if( len + fqPos >= (unsigned int)fmtCapacity )
			{
				// Enlarge
				fmtCapacity += len + FQINC;
				fmtQuery = (char *)realloc( fmtQuery, fmtCapacity+1 );
			}

			strcat( fmtQuery, quoted );
			free( quoted );
			fqPos += len;
		}
		else if( val->IsInt32() )
		{
			long long intv = Nan::To<int32_t>(val).FromJust();
			char numStr[32];
			snprintf( numStr, 32, "%lli", intv );
			int len = strlen(numStr);
			
			if( len + fqPos >= fmtCapacity )
			{
				// Enlarge
				fmtCapacity += len + FQINC;
				fmtQuery = (char *)realloc( fmtQuery, fmtCapacity+1 );
			}

			strcat( fmtQuery, numStr );
			fqPos += len;
		}
		else if( val->IsUint32() )
		{
			unsigned long long intv = Nan::To<uint32_t>(val).FromJust();
			char numStr[32];
			snprintf( numStr, 32, "%llu", intv );
			int len = strlen(numStr);
			
			if( len + fqPos >= fmtCapacity )
			{
				// Enlarge
				fmtCapacity += len + FQINC;
				fmtQuery = (char *)realloc( fmtQuery, fmtCapacity+1 );
			}

			strcat( fmtQuery, numStr );
			fqPos += len;
		}
		else // FIXME: If it isn't a string or int32, it's a 'number', I guess:
		{
			double dbl = Nan::To<double>(val).FromJust();
			char numStr[32];
			snprintf( numStr, 32, "%f", dbl );
			int len = strlen(numStr);
			
			if( len + fqPos >= fmtCapacity )
			{
				// Enlarge
				fmtCapacity += len + FQINC;
				fmtQuery = (char *)realloc( fmtQuery, fmtCapacity+1 );
			}

			strcat( fmtQuery, numStr );
			fqPos += len;
		}

		// Don't "lose" the character following an index!
		if( !atEnd )
		{
			fmtQuery[ fqPos++ ] = c;
			fmtQuery[ fqPos ] = '\0';
		}
	}

	// This method isn't threadsafe:
	pthread_mutex_lock( &st_mutex );
	m_result = DBI::dbi_conn_query( m_parent->m_conn, fmtQuery );
	pthread_mutex_unlock( &st_mutex );

	free( fmtQuery );
	free( rawQuery );
	if( !m_result )
		return false;
	return true;
}

void DBQuery::Count(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	v8::Local<v8::Number> num = v8::Number::New( isolate, DBI::dbi_result_get_numrows( obj->m_result ) );

	args.GetReturnValue().Set(num);
}

void DBQuery::FieldCount(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	v8::Local<v8::Number> num = v8::Number::New( isolate, DBI::dbi_result_get_numfields( obj->m_result ) );

	args.GetReturnValue().Set(num);
}

void DBQuery::FieldName(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	if (args.Length() < 1) {
		isolate->ThrowException(v8::Exception::TypeError( v8str("A field index is required.") ) );
		return;
	}

	if (!args[0]->IsNumber()) {
		isolate->ThrowException(v8::Exception::TypeError( v8str("A numeric field index is required.") ) );
		return;
	}

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	unsigned int idx = args[0]->NumberValue();
	if( idx < 1 || idx > DBI::dbi_result_get_numfields( obj->m_result ) )
	{
		isolate->ThrowException(v8::Exception::TypeError( v8str("Index is invalid.") ) );
		return;
	}

	const char *fname = DBI::dbi_result_get_field_name( obj->m_result, idx );
	args.GetReturnValue().Set( v8str(fname) );
}

void DBQuery::FieldIndex(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	if (args.Length() < 1) {
		isolate->ThrowException(v8::Exception::TypeError( v8str("A field name is required.") ) );
		return;
	}

	if (!args[0]->IsString()) {
		isolate->ThrowException(v8::Exception::TypeError( v8str("Provided field name must be a string.") ) );
		return;
	}

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	v8::Local< v8::String > fieldName = args[0]->ToString();
	
	char *fieldChar = new char[ fieldName->Length() + 1 ];
	fieldName->WriteUtf8( fieldChar );
	unsigned int idx = DBI::dbi_result_get_field_idx( obj->m_result, fieldChar );
	delete fieldChar;

	v8::Local<v8::Number> num = v8::Number::New( isolate, idx );
	args.GetReturnValue().Set( num );
}

void DBQuery::Seek(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	if (args.Length() < 1 || !args[0]->IsNumber())
	{
		isolate->ThrowException(v8::Exception::TypeError( v8str("A numeric row index is required.") ) );
		return;
	}

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	unsigned long long idx = args[0]->NumberValue();
	int result = DBI::dbi_result_seek_row( obj->m_result, idx );
	args.GetReturnValue().Set( v8::Boolean::New( isolate, result == 1 ? true : false ) );
}

void DBQuery::PreviousRow(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	int result = DBI::dbi_result_prev_row( obj->m_result );
	args.GetReturnValue().Set( v8::Boolean::New( isolate, result == 1 ? true : false ) );
}

void DBQuery::NextRow(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	int result = DBI::dbi_result_next_row( obj->m_result );
	args.GetReturnValue().Set( v8::Boolean::New( isolate, result == 1 ? true : false ) );
}

void DBQuery::CurrentRow(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	unsigned long long result = DBI::dbi_result_get_currow( obj->m_result );
	args.GetReturnValue().Set( v8::Number::New( isolate, result ) );
}

void DBQuery::Value(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	if( args.Length() < 1 )
	{
		isolate->ThrowException(v8::Exception::TypeError( v8str("A numeric field index or field name string is required.") ) );
		return;
	}

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	unsigned int idx;
	if( args[0]->IsNumber() )
		idx = args[0]->NumberValue();
	else if( args[0]->IsString() )
	{
		v8::Local< v8::String > fieldName = args[0]->ToString();
	
		char *fieldChar = new char[ fieldName->Length() + 1 ];
		fieldName->WriteUtf8( fieldChar );
		idx = DBI::dbi_result_get_field_idx( obj->m_result, fieldChar );
		delete fieldChar;
	}
	else
	{
		isolate->ThrowException(v8::Exception::TypeError( v8str("First parameter must be either a numeric field index or field name string.") ) );
		return;
	}

	if( idx < 1 || idx > DBI::dbi_result_get_numfields( obj->m_result ) )
	{
		isolate->ThrowException(v8::Exception::TypeError( v8str("Index is invalid.") ) );
		return;
	}

	unsigned short type = DBI::dbi_result_get_field_type_idx( obj->m_result, idx );
	if( DBI_TYPE_ERROR == type )
	{
		const char *msg;

		// This method isn't threadsafe:
		pthread_mutex_lock( &st_mutex );
		DBI::dbi_conn_error( obj->parent()->m_conn, &msg );
		pthread_mutex_unlock( &st_mutex );

		isolate->ThrowException(v8::Exception::TypeError( v8::String::Concat( v8str("For some reason I can't figure out the data storage type used by this field: "), v8str(msg) ) ) );
		return;
	}

	if( DBI_TYPE_INTEGER == type )
	{
		// FIXME: This really should handle short, long, long long, signed, and unsigned specially. Parse attribs for this:
		v8::Local<v8::Number> num = v8::Number::New( isolate, DBI::dbi_result_get_longlong_idx( obj->m_result, idx ) );
		args.GetReturnValue().Set(num);
		return;
	}
	else if( DBI_TYPE_DECIMAL == type )
	{
		v8::Local<v8::Number> num = v8::Number::New( isolate, DBI::dbi_result_get_double_idx( obj->m_result, idx ) );
		args.GetReturnValue().Set(num);
		return;
	}
	else if( DBI_TYPE_STRING == type )
	{
		char *str = DBI::dbi_result_get_string_copy_idx( obj->m_result, idx );
		if( !str )
		{
			args.GetReturnValue().Set( v8::Null(isolate) );
			return;
		}

		v8::Local<v8::String> vstr = v8str(str);
		args.GetReturnValue().Set( vstr );
		free( str );
		return;
	}
	else if( DBI_TYPE_DATETIME == type )
	{
		args.GetReturnValue().Set( v8::Date::New( isolate, DBI::dbi_result_get_datetime_idx( obj->m_result, idx ) ) );
		return;
	}
	else if( DBI_TYPE_BINARY == type )
	{
		size_t length = DBI::dbi_result_get_field_length_idx( obj->m_result, idx );
		const unsigned char *data = DBI::dbi_result_get_binary_idx( obj->m_result, idx );
		if( !data )
		{
			args.GetReturnValue().Set( v8::Null(isolate) );
			return;
		}

#if( NODE_MAJOR_VERSION == 0 )
		v8::Local< v8::Object > bufObj = node::Buffer::New(isolate, (const char *)data, length);
#else
		v8::MaybeLocal< v8::Object > mlocObj = node::Buffer::Copy(isolate, (const char *)data, length);
		v8::Local< v8::Object > bufObj;
		if( mlocObj.IsEmpty() || !mlocObj.ToLocal( &bufObj ) )
		{
			isolate->ThrowException(v8::Exception::TypeError( v8str("Failed to allocate a new Buffer for binary data result.") ) );
			return;
		}
#endif

		args.GetReturnValue().Set( bufObj );

		return;
	}

	isolate->ThrowException(v8::Exception::TypeError( v8str("Sorry, the data type at this field index isn't supported for some reason.") ) );
	return;
}

void DBQuery::Commit(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	int result = DBI::dbi_conn_transaction_begin( obj->parent()->m_conn );
	args.GetReturnValue().Set( v8::Boolean::New( isolate, result == 0 ? true : false ) );
}

void DBQuery::Rollback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	int result = DBI::dbi_conn_transaction_rollback( obj->parent()->m_conn );
	args.GetReturnValue().Set( v8::Boolean::New( isolate, result == 0 ? true : false ) );
}

void DBQuery::Begin(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBQuery* obj = Unwrap<DBQuery>(args.Holder());
	int result = DBI::dbi_conn_transaction_begin( obj->parent()->m_conn );
	args.GetReturnValue().Set( v8::Boolean::New( isolate, result == 0 ? true : false ) );
}

