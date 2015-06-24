var nodedbi = require('./build/Release/nodedbi.node');
/*
typedef enum {
  DBI_ERROR_USER = -10,
  DBI_ERROR_DBD = -9,
  DBI_ERROR_BADOBJECT,
  DBI_ERROR_BADTYPE,
  DBI_ERROR_BADIDX,
  DBI_ERROR_BADNAME,
  DBI_ERROR_UNSUPPORTED,
  DBI_ERROR_NOCONN,
  DBI_ERROR_NOMEM,
  DBI_ERROR_BADPTR,
  DBI_ERROR_NONE = 0,
  DBI_ERROR_CLIENT
} dbi_error_flag;
*/
nodedbi['DBI_ERROR_USER'] = -10;
nodedbi['DBI_ERROR_DBD'] = -9;
nodedbi['DBI_ERROR_BADOBJECT'] = -8;
nodedbi['DBI_ERROR_BADTYPE'] = -7;
nodedbi['DBI_ERROR_BADIDX'] = -6;
nodedbi['DBI_ERROR_BADNAME'] = -5;
nodedbi['DBI_ERROR_UNSUPPORTED'] = -4;
nodedbi['DBI_ERROR_NOCONN'] = -3;
nodedbi['DBI_ERROR_NOMEM'] = -2;
nodedbi['DBI_ERROR_BADPTR'] = -1;
nodedbi['DBI_ERROR_NONE'] = 0;
nodedbi['DBI_ERROR_CLIENT'] = 1;
module.exports = nodedbi;
