# DropboxFS

Acest proiect implementează un sistem de fișiere simplu, utilizând FUSE (Filesystem in Userspace), care interacționează cu Dropbox. Permite utilizatorilor să creeze directoare, fișiere și să scrie conținut în fișiere, sincronizând automat aceste modificări în Dropbox folosind utilitarul `dbxcli`.

---

## Funcționalități

- **Operatii pe Directoare**
  - Crearea directoarelor (`mkdir`) și încărcarea acestora în Dropbox.
- **Operatii pe Fișiere**
  - Crearea fișierelor și încărcarea acestora în Dropbox.
  - Scrierea în fișiere și sincronizarea conținutului în Dropbox.
- **Gestionarea Conținutului Fișierelor**
  - Stocarea conținutului fișierelor în memorie.
  - Recuperarea metadatelor fișierelor și listarea directoarelor.

---

## Cerințe Prealabile

Asigurați-vă că următoarele pachete și instrumente sunt instalate înainte de rularea programului:

### Instalarea Dropbox CLI (`dbxcli`)
```bash
sudo wget "https://github.com/dropbox/dbxcli/releases/download/v3.0.0/dbxcli-linux-amd64"
sudo mv dbxcli-linux-amd64 dbxcli
sudo chmod +x dbxcli
sudo mv dbxcli /usr/local/bin
```
#### Autentificare în dbxcli
- Crează un cont de Dropbox;
- Folosește comanda `dbxcli account`;
- Accesează link-ul dat de comandă;
- Permite accesul utilitarului la cont, copiază cheia dată și scri-o în terminal;


### Instalare FUSE
```bash
sudo apt update
sudo apt install libfuse-dev
sudo apt install fuse
sudo apt-get install pkg-config
```
**ATENȚIE!  Aceasta bibliotecă nu este compatibilă cu versiunile recente de Ubuntu. Instalarea acesteia duce la distrugerea sistemului de operare.** (Am pățit (de cateva ori))

## Compilarea și executarea proiectului
Pentru a executa proiectul, este nevoie să faci mount sistemului de fișiere din Dropbox într-un folder local.
``` bash
gcc lsysf.c -o lsysf 'pkg-config fuse --cflags --libs'
./lsysf -f ~/[folder-ul în care dorești să faci mount]
```
Pentru executarea operațiilor, este nevoie să deschizi un nou terminal în folder-ul în care ai făcut mount.
## Realizarea unmount a sistemului de fișiere:
``` bash
fusermount -u ~/[folder-ul în care ai făcut mount]
```
Pentru ca această comandă să funcționeze, trebuie să ieși din folder-ul în care ai făcut mount în terminalul în care ai executat operații pe sistemul de fișiere. Execuția programului se încheie odată cu rularea comenzii de unmount.
 



