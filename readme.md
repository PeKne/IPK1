# IPK1: klient-server pro přenos souborů

Jednoduchá aplikace přenášející soubory mezi klientem a serverem.

## Sestavení
Pro překlad, v pracovním adresáři spustě Makefile pomocí příkazu 'make'

```
user@machine:~$ make
  gcc  ipk-client.c -o ipk-client
  gcc  ipk-server.c -o ipk-server
```
## Spuštění

### SERVER
Server se spouštím s jedním povinným parametre čísla portu
```
user@machine:~$ ./ipk-server -p 'port_number'
```

### KLIENT
Klient má tři povinné parametry:
*  -h [nazev hosta]
*  -p [cislo portu]
*  -w nebo -r [nazev souboru] (pro zapis nebo cteni)
```
./ipk-client -h merlin.fit.vutbr.cz -p 'port_number' -r filename.txt
```
## Autor

* **Petr Knetl** (xknetl00)
