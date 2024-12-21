# Documentație Proiect: Sistem de Fisiere FUSE cu Dropbox

## Prezentare Generală

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
sudo dbxcli account
```
### Instalare FUSE
```bash
sudo apt update
sudo apt install libfuse-dev
sudo apt install fuse
sudo apt-get install pkg-config
```
**ATENȚIE!  Aceasta bibliotecă nu este compatibilă cu versiunile recente de Ubuntu. Instalarea acesteia duce la distrugerea sistemului de operare.** (Am pățit (de cateva ori))




