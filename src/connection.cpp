#define BUILDING_NODE_EXTENSION

#include <node/v8.h>
#include <node/node.h>
#include <node/node_object_wrap.h>

#include <string.h>
#include <stdlib.h>

using namespace v8;

#include "connection.h"
#include "query.h"

static DBI::dbi_inst dbi_instance;
static bool dbi_initialised = false;

v8::Persistent< v8::Function > DBConnection::constructor;

void DBConnection::Init( v8::Handle<v8::Object> exports )
{
	// Initialise if needed. Ye olde C-style.
	if( !dbi_initialised )
	{
		DBI::dbi_initialize_r(NULL, &dbi_instance);
		dbi_initialised = true;
	}

	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	
	// Prepare constructor template
	v8::Local<FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
	tpl->SetClassName(v8str("DBConnection"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "query", Query);
	NODE_SET_PROTOTYPE_METHOD(tpl, "lastError", LastError);
	NODE_SET_PROTOTYPE_METHOD(tpl, "lastErrorCode", LastErrorCode);
	
	constructor.Reset(isolate, tpl->GetFunction());
	exports->Set(v8str("DBConnection"),
	tpl->GetFunction());
}

void DBConnection::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	if( !args.IsConstructCall() )
	{
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		const int argc = 1;
		v8::Local<v8::Value> argv[argc] = { args[0] };
		v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
		args.GetReturnValue().Set(cons->NewInstance(argc, argv));
		return;
	}

	//Invoked as constructor: `new MyObject(...)`
	if (args.Length() < 1) {
		isolate->ThrowException(v8::Exception::TypeError(v8str("Wrong number of arguments.")));
		return;
	}

	DBConnection *dbObj = new DBConnection();
	if( !dbObj->initialise( args ) )
	{
		delete dbObj;
		return;
	}

	dbObj->Wrap(args.This());
	args.GetReturnValue().Set(args.This());
}

bool DBConnection::initialise( const v8::FunctionCallbackInfo<v8::Value>& args )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	v8::Local< v8::Object > arg = args[0]->ToObject();
	v8::Local< v8::Value > vkey, vval;
	char ckey[32], cval[64];

	v8::Local< v8::Value > typeVal = arg->Get( v8str("type") );
	if( typeVal->IsNull() )
	{
		isolate->ThrowException(v8::Exception::TypeError(v8str("A 'type' parameter specifying the database type is required.")));
		return false;
	}

	v8::Local< v8::String > tstr = typeVal->ToString();
	if( tstr->Length() == 0 )
	{
		isolate->ThrowException(v8::Exception::TypeError(v8str("A 'type' parameter specifying the database type is required.")));
		return false;
	}

	tstr->WriteUtf8( cval, sizeof(cval) );
	m_conn = DBI::dbi_conn_new_r( cval, dbi_instance );

	v8::Local< v8::Array > propNames = arg->GetPropertyNames();

	for( unsigned int x=0; x < propNames->Length(); x++ )
	{
		vkey = propNames->Get(x);
		vval = arg->Get( vkey );

		vkey->ToString()->WriteUtf8( ckey, sizeof(ckey) );
		vval->ToString()->WriteUtf8( cval, sizeof(cval) );

		if( strcmp( ckey, "type" ) == 0 )
		{
			// Already did this:
			continue;
		}
		
		DBI::dbi_conn_set_option( m_conn, ckey, cval );
	}

	// Attempt to connect why not.
	if( DBI::dbi_conn_connect(m_conn) < 0 )
	{
		isolate->ThrowException( v8::Exception::TypeError( v8str("Connection failed") ) );
		m_conn = NULL;

		return false;
	}

	return true;
}

void DBConnection::Query( const v8::FunctionCallbackInfo<Value>& args )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	if (args.Length() < 1) {
		isolate->ThrowException(v8::Exception::TypeError(
			v8str("At least a query must be provided.")));
		return;
	}

	if (!args[0]->IsString()) {
		isolate->ThrowException(v8::Exception::TypeError(
			v8str("First argument should be a string.")));
		return;
	}

	DBConnection* parent = Unwrap<DBConnection>(args.Holder());

	DBQuery *obj = new DBQuery( parent );

	// Attempt the query early. If it fails, don't do extra work, just return 'false'.
	if( !obj->query( args ) )
	{
		delete obj;
		args.GetReturnValue().Set( v8::Boolean::New(isolate, false) );
		return;
	}

	obj->Init(args);
}

void DBConnection::LastError(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBConnection* obj = Unwrap<DBConnection>(args.Holder());
	const char *err;

	// This method isn't threadsafe.
	pthread_mutex_lock( &st_mutex );
	DBI::dbi_conn_error( obj->m_conn, &err );
	pthread_mutex_unlock( &st_mutex );

	args.GetReturnValue().Set( v8str( err ) );
}

void DBConnection::LastErrorCode(const v8::FunctionCallbackInfo<v8::Value>& args)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	DBConnection* obj = Unwrap<DBConnection>(args.Holder());

	// This method isn't threadsafe.
	pthread_mutex_lock( &st_mutex );
	int code = DBI::dbi_conn_error( obj->m_conn, NULL );
	pthread_mutex_unlock( &st_mutex );

	v8::Local<v8::Number> num = v8::Number::New( isolate, code );
	args.GetReturnValue().Set( num );
}

DBConnection::DBConnection() :
	m_conn(NULL)
{
}

DBConnection::~DBConnection()
{
	if( m_conn )
		DBI::dbi_conn_close(m_conn);
}

// "Actual" node init:
static void init(v8::Handle<v8::Object> exports) {
	DBConnection::Init(exports);
}

// @see http://github.com/ry/node/blob/v0.2.0/src/node.h#L101
NODE_MODULE(nodedbi, init);
