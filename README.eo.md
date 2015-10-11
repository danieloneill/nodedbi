# NodeDBI

NodeDBI estas LibDBI interfaco por Node.js.

Por la fontkodo bonvolu vizitu [la github pagxo](https://github.com/danieloneill/nodedbi)

La NodeDBI interfaco liveras tradiciajn metodojn por uzi SQL datumbazoj, kaj ankaux la kapablo por pagxi rezultatojn kun persistaj objektoj. Vidu la suban ekzemplojn.

Gxi estas plejparte plena, kaj kontribuoj estas tre aprezas.

Por uzi tio cxi, vi deve havas LibDBI-n kaj gxiajn rubrikojn.

Por Debian aux Ubuntu, **apt-get install libdbi-dev**.

Por konstrui, tajpu: **node-gyp configure build** kaj **node-gyp install** por instali.

## Ekzemploj

Mi havas kelke da Gistojn kiu eble uza:

* [node.js + nodedbi, oportunaj metodoj](https://gist.github.com/danieloneill/2605640f020c89fb806a)
* [Nodejs + libdbi + pagxi rezultatoj](https://gist.github.com/danieloneill/d069be8e02e852008cbd)

```javascript
#!/usr/bin/nodejs

var mod = require('nodedbi');

var args = { 'host':'localhost', 'port':3306, 'username':'root', 'password':'', 'type':'mysql', 'dbname':'test' };

var obj = new mod.DBConnection( args );

var q = obj.query("SELECT * FROM users WHERE username=%1 OR id=%2", ['doneill', 6]);
if( !q )
{
        console.log("SQL peto malsukcesis: "+obj.lastError());
        return;
}

// Simple:
var results = q.toArray();

// Malsimple, se eble rapida:
var rc = q.count();
console.log("Nombro da vicoj: "+rc);

var fc = q.fieldCount();
console.log("Nombro da kolumnoj: "+q.fieldCount());

var fna = [];
for( var x=0; x < fc; x++ )
{
        var fn = q.fieldName(x+1);
        console.log("Rubrikonomo ("+(x+1)+"): "+fn);
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

```

# Metodoj

## DBConnection( parametroj )
`Verku novan konekton al datumbazon.`

```javascript
var NodeDBI = require('nodedbi');
var parametroj = { 'type':'sqlite', 'dbname':'test' };
var db = new NodeDBI.DBConnection(parametroj);
```

La parametro **type**  (en parametroj) estas deva. Ioj parametroj estas datumbazo specifa. Por datumbazo specifa parametroj vizitu [la libdbi-drivers pagxo](http://libdbi-drivers.sourceforge.net/docs.html).

* `parametroj` - Objekto kun LibDBI-specifaj konektaj parametroj en sxlosilo:valoro paroj.
* `Revenos`: Datumbaza objekto, aux **undefined** se malsukcese.

---

## DBConnection::query(petoLiteraro, [parametroj])
`Verku kaj ekzekutu datumbazan peton.`

* `petoLiteraro` - Peto liter-ar-o kun lokokupiloj por parametroj %1, %2, ktp.
* `parametroj` - Faktultativa listo de parametroj por la lokokupiloj.
* `Revenos`: DBQuery objekto, aux **false** se la peton malsukcesas por ia kialo.

---

## DBConnection::lastError()
`Revenu la plejnovan eraron literaron.`

* `Revenos`: Supre menciita.

---

## DBConnection::lastInsertId([nomo])
`La aktualan vicen legitimilon generis de la antauxa INSERT komando.`

* `nomo` - La nomon de la vico ("sequence", ne "row",) aux NULL se la datumbazon ne havas vicojn eksplicitan.
* `Revenos`: La aktualan vicen legitimilon generis de la antauxa INSERT komando. Se la datumbazo ne uzi vicojn, tio cxi revenos 0.

---

## Query()
`Tio cxi objekto estas revenas per` **DBConnection::query** `kaj malpermesas verki de mem alie.`

---

## Query::count()
`Revenu la nombron da vicoj de la rezultaro.`

* `Revenos`: Supre menciita, havas unu da taskon.

---

## Query::fieldCount()
`Revenu la nombron da kolumnoj de la rezultaro.`

* `Revenos`: Supre menciita.

---

## Query::fieldName(kolumnIndekso)
`Revenu la kampo-nomon de specifa kolomno-indekso. Indeksoj komencas cxe` **1** `, ne 0.`

* `kolumnIndekso` - La kolumno-indekso de la rezultan objekton. Kolumno-indeksoj en LibDBI komencas cxe 1, ne 0.
* `Revenos`: La nomon de la kampo cxe la specifike indekson aux **undefined** kaj escepto se la indekso estas malvalida.

---

## Query::fieldIndex(kampnomo)
`Revenu la kampo-indekso de specifa kampo-nomon. Indeksoj komencas cxe` **1** `, ne 0.`

* `kampnomo` - La kampo-nomo de la rezultan objekton.
* `Revenos`: La kolomnon-indekson kun la specifike kampo-nomo aux **undefined** kaj escepto se la nomon maltrovas. Kolumnejn indeksojn en LibDBI komencas cxe **1**, ne 0.

---

## Query::seek(vico)
`Muvu al la specifan vicon en la rezultaro. Vicen indeksojn en LibDBI komencas cxe` **1** `, ne 0.`

* `vico` - La indekso en la rezultaro. Vicejn indeksojn komencas cxe **1**, ne 0.
* `Revenos`: **true** por sukcese, **false** aux escepto se malsukcese.

---

## Query::previousRow()
`Muvu al la antauxan vicon en la rezultaro.`

* `Revenos`: **true** por sukcese, **false** aux escepto se gxi jam al la unuan rezulton.

---

## Query::nextRow()
`Muvu al la sekvan vicon en la rezultaro.`

* `Revenos`: **true** por sukcese, **false** aux escepto se gxi jam al la lastan rezulton.

---

## Query::currentRow()
`La aktuala vicindekso, komencigxi al 1.`

* `Revenos`: La aktualan vicen indekson. Vicejn indeksojn komenci cxe **1**, ne 0.

---

## Query::value(kampo)
`La valoro de la kampo al la aktuala vico de la rezultaro.

**kampo** rajtas estu aux numere indekso aux kampnomo.

Valorojn estas konvertis al la plej bone Javascript tipo. Binara datumoj estas revenas kiel [Buffer-objekton](https://nodejs.org/api/buffer.html).

* `Revenos`: La valoron.

---

## Query::begin()
`Komencu transakcio.`

* `Revenos`: **true** por sukcese, **false** se malsukcese aux se la datumbazo ne uzas (aux ne havas) transacia subteni.

---

## Query::commit()
`Commits a transaction, i.e. writes all changes since the transaction was started to the database.`

* `Returns`: **true** on success, **false** if the database cannot or will not commit the transaction.

---

## Query::rollback()
`Rolls back a transaction, i.e. reverts all changes since the transaction started.`

* `Returns`: **true** on success, **false** if the database cannot or will not commit the transaction.

