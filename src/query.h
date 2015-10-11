namespace DBI {
 extern "C" {
# include <dbi.h>
 }
}

class DBConnection;
class DBQuery : public node::ObjectWrap {
	private:
		DBConnection    *m_parent;
		DBI::dbi_result m_result;

	public:
		DBQuery( DBConnection *parent );
		~DBQuery();

		DBConnection *parent() { return m_parent; }

		bool query( const v8::FunctionCallbackInfo<v8::Value>& args );

		// v8 stuff:
		void Init( const v8::FunctionCallbackInfo<v8::Value>& args );
		static void Count(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void FieldCount(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void FieldName(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void FieldIndex(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Seek(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void PreviousRow(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void NextRow(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void CurrentRow(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Value(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Begin(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Commit(const v8::FunctionCallbackInfo<v8::Value>& args);
		static void Rollback(const v8::FunctionCallbackInfo<v8::Value>& args);
};

