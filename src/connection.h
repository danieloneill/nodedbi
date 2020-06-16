#include <nan.h>
#include <pthread.h>

namespace DBI {
 extern "C" {
# include <dbi.h>
 }
}

#define v8str(a) Nan::New<v8::String>(a).ToLocalChecked()

static pthread_mutex_t st_mutex;

class DBQuery;
class DBConnection : public node::ObjectWrap {
	static v8::Persistent< v8::Function > constructor;

        protected:
	friend class DBQuery;

		DBI::dbi_conn m_conn;

        public:
                DBConnection();
                ~DBConnection();

		bool initialise( const v8::FunctionCallbackInfo< v8::Value > &args );
	
		// v8 stuff:
                static void Init(v8::Local<v8::Object> exports);
		static void New(const v8::FunctionCallbackInfo< v8::Value >& args);
		static void Query( const v8::FunctionCallbackInfo< v8::Value >& args );
		static void LastError( const v8::FunctionCallbackInfo< v8::Value >& args );
		static void LastErrorCode( const v8::FunctionCallbackInfo< v8::Value >& args );
		static void LastInsertID(const v8::FunctionCallbackInfo<v8::Value>& args);
};

