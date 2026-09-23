# Bojanka

Jednostavna Paint aplikacija za **Helen OS** / **SrBin OS**, inspirisana prvim Windows Paint-om (MS Paint 1.0, 1985).

![Bojanka screenshot](images/screenshot.png)

---

## Funkcionalnosti

### Alati za crtanje
- **Slobodno crtanje** (olovka)
- **Prava linija**
- **Prazan pravougaonik**
- **Pun pravougaonik**
- **Prazan krug**
- **Pun krug**
- **Flood fill** (popuni bojom)

### Debljina četke
- 5 nivoa: **1, 2, 3, 4, 5 px**

### Paleta boja
- 8 osnovnih boja: **crna, bela, crvena, zelena, plava, žuta, narandžasta, braon**

### Fajlovi
- **Sačuvaj** — čuva u `.boj` format (RLE kompresija)
- **Sačuvaj kao** — dijalog za unos imena
- **Otvori** — dijalog za izbor fajla

### Undo / Redo
- **10 koraka** undo/redo
- **RLE kompresija** u memoriji (bez fajlova)
- Brzo i efikasno — tipično **2–10 KB po koraku**

### Ostalo
- Statusna traka sa koordinatama miša i trenutnom bojom
- Meniji: **Dokument, Akcije, Veličina, Boja, Oblik, Pomoć**
- Podrazumevano: **olovka, 2px, crna**

---

## Screenshots

### Glavni prozor
![Main window](images/screenshot.png)

### Meni
![Menu](images/menu.png)

### Crtanje
![Drawing](images/drawing.png)

---

## Format `.boj`

Bojanka koristi sopstveni format za čuvanje slika — **RLE kompresovan** niz piksela:

