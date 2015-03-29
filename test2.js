var LibDBI = require('./build/Release/nodedbi.node');

var db = LibDBI.DBConnection({'type':'mysql', 'dbname':'mysql', 'username':'root'});

function getResultHandle(query, args)
{
  var q = db.query(query, args);
  if( !q )
  {
    console.log( "Query error: " + db.lastError() );
    return false;
  }
  
  q.getRows = function(start, end)
  {
    var rows = [];
    var rowCount = this.count();
    var colCount = this.fieldCount();
    
    if( !end )
      end = start;
    
    if( start < 1 || start > rowCount )
    {
        console.log( "getRows(): 'start' is outside of valid result range!");
        return false;
    }
    
    if( end < 1 || end > rowCount )
    {
        console.log( "getRows(): 'end' is outside of valid result range!");
        return false;
    }
    
    if( start > end )
    {
        console.log( "getRows(): for now, 'start' must be a lower index than 'end'");
        return false;
    }
    
    for( var x=start; x <= end; x++ )
    {
      if( !this.seek(x) )
      {
        console.log( "getRows(): seek failed at index "+x);
        return false;
      }
    
      var row = [];
      for( var y=1; y <= colCount; y++ )
        row.push( this.value(y) );
      
      rows.push( row );
    }
    
    return rows;
  }

  return q;
}

var q = getResultHandle("SELECT * FROM user");
var rowCount = q.count();

// Store q somewhere, it takes very little RAM.
// Eventually you may want a row, in this case the second:
var row = q.getRows(2); 
console.log( JSON.stringify(row, null, 2) );

delete q;

