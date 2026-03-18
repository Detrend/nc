## Plany na pokracovanie projektu Nuclidean
- Chceme obhajovat na MFF koncom Aprila
- Chceli by sme na projekte pokracovat dalej
- Viac krat sme sa bavili co by sme mohli robit dalej, ale nikam sme to oficialne nenapisali
- Tak som zalozil tento dokument

---

### Jaro & Art
- Jarovi by pomohlo pridavat art jednoduchsie do hry
- Diskutovali sme na pive a neprebrali sme do hlbky
- **Treba prebrat o co presne by islo**

---

### Tech Art
- Consistent pixel size across all textures
  - Pixels on all textures would have the same world size (for example 3x3 cm = pixel)
  - Would be consistent everywhere
- Pixel locked lighting
  - The light value is the same across the whole pixel
- Vulkan & more GFX backends
  - Chceme do buducna aj graficky zlozitejsie backendy
  - Chceme podporovat aj starsie PC -> nepotrebujes RTX kartu
- Lepsi global illumination
  - Viacej sposobov ako sa k tomu dostat

---

### Design
- Vacsie levely
- Detailnejsie levely
  - Nie v pocte pixelov, ale v kvalite prostredia
  - Napr. steny niesu jedna opakujuca sa textura, ale skladaju sa z viacej patternov
  - Prostredie vyzera viacej ako z realneho sveta, cize napr. bude obsahovat propy pre predmety, objekty atd
  - Prostredie dava "zmysel" - posobi ako skutocne miesta, budovy a miestnosti a nie iba ako zopar narychlo poskladanych polygonov

---

### Linux Port
- Chceme

---

### Co-Op mode
- Pre experiment by som to skusil
- Predstava je ze by sa dvaja (alebo viaceri?) hraci mohli spojit cez siet a hrat spolu co-op levely
- Technicky by to bolo hacknute, neslo by o "fancy" sposob ako riesit multiplayer
- Vacsine tazkych problemov ako komplexnu synchronizaciu by sme hackli alebo snazili obist

---

### Marketing
- Chceme zviditelnit nasu hru
- Pretoze nechceme Nuclidean releasnut po 2 rokoch developmentu a sledovat jak si ho nikto nezahra
- Zatial neprediskutovane, ale je tu viacej moznosti
  - Devlogs
    - Jara navrhoval, ze by s nimi mohol pomoct
    - Mohli by obsahovat napr. technicke detaily alebo zaujimavosti
    - Ako funguje lighting alebo enemies
    - Ako funguju portaly
  - Idealne mat socialne siete kam by sme zbierali followers - YouTube, Instagram, atd..
  - Potrebujeme zbierat Steame wishlists - extremne dolezite, ked zacne mat hra viacej wishlists, tak ju Steam viacej odporuca
  - Potrebujeme dobry rating na Steame - hry s vysokym hodnotenim su viacej odporucane
  - Mozeme poslat zdrojaky na "review" nejakemu YouTuberovi - napr. The Cherno robi code review pre vlastne C++ hry a ma pomerne vela zhliadnuti na videach

---

### Open Source
- Chceme formu "open source"
  - Ale skor "read only" verziu
  - Nechcem a neocakavam od fanusikov, ze budu hru fixovat alebo dokoncovat za nas - to ma byt praca developera
  - Cize by neslo nutne o open source typu "kazdy na svete moze do Nuclideana pridat co sa mu zachce"
  - Ale kazdy by si mohol spravit vlastny fork
- Treba ale vyriesit problemy ako
    - aby nemohol hocikto prekompilovat a znovu predavat hru za nas

---

### Business
- Chceme predavat hru na Steame
- Mozno by sme chceli mat free demo verziu a potom platenu full verziu?
- Paci sa nam cena okolo 20€
- Pre predstavu - Dusk a Hrot, ktore si obe pytaju 20€, maju avg. playtime 5 & 7 hodin
