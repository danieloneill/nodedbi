#!/usr/bin/nodejs

var mod = require('./build/Release/nodedbi.node');

var args = { 'host':'localhost', 'port':3306, 'username':'root', 'password':'', 'type':'mysql', 'dbname':'test' };

var obj = new mod.DBConnection( args );

function doQuery()
{
	var q = obj.query("SELECT * FROM users WHERE username=%1 OR id=%2", ['doneill', 6]);
	if( !q )
	{
		console.log("Query failed!");
		return;
	}

	var rc = q.count();
	console.log("Row count: "+rc);
	
	var fc = q.fieldCount();
	console.log("Field count: "+q.fieldCount());
	
	var fna = [];
	for( var x=0; x < fc; x++ )
	{
		var fn = q.fieldName(x+1);
		console.log("Field name("+(x+1)+"): "+fn);
		fna.push( fn );
	}
	
	for( var y=0; y < rc; y++ )
	{
		q.seek(y+1);
		for( var x=0; x < fc; x++ )
		{
			var v = q.value(x+1);
			console.log( y+" ["+fna[x]+"] = ["+v+"]" );
		}
	}
}

doQuery();

