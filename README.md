# ⚖️ Balance Connectée NB-IoT  
### Système de pesée autonome, étanche et solaire basé sur TTGO SIM7080G

![GitHub Repo](https://img.shields.io/badge/GitHub-Ratamuse--Beehive__scale__Nb--iot__V3-blue?logo=github)
![Langue](https://img.shields.io/badge/Langue-Français-blue)
![Version](https://img.shields.io/badge/Version-v1.0-green)
![Licence](https://img.shields.io/badge/Licence-MIT-lightgrey)
![Auteur](https://img.shields.io/badge/Auteur-Ratamuse-orange)

---

## 📚 Table des matières

1. [Présentation du projet](#presentation-du-projet)
2. [Composants — Boîtier électronique](#liste-des-composants-boitier-electronique)
3. [Composants — Structure de la balance](#liste-des-composants-structure-de-la-balance)
4. [Estimation des coûts (5 stations et 10 balances)](#estimation-des-couts-5-stations-et-10-balances)
5. [Étapes d’assemblage](#etapes-dassemblage)
6. [Téléchargement](#telechargement)
7. [Firmware](#firmware)
8. [Mode d'emploi](#mode-demploi)
9. [Partie Web et IoT](#web-iot)
10. [Licence](#licence)
11. [Auteur](#auteur)


---

<a name="presentation-du-projet"></a>
## 🧠 Présentation du projet

Balances connectées au réseau **NB-IoT** via la carte **TTGO SIM7080G** de LilyGO.  
Cette carte embarque :

- un **MCU ESP32-S3**
- un modem **NB-IoT SIM7080G**
- un **chargeur solaire** pour batterie **18650 Li-Ion**
- un **GPS**

Les balances sont branchées à la TTGO via des convertisseurs **ADC 24 bits HX711**.  
Une sonde de température **DS18B20** complète l’ensemble.

Pour faciliter les branchements, un **circuit imprimé dédié** est disponible. Il comprend :

- 2 convertisseurs **HX711**
- tous les connecteurs nécessaires (balances, sonde, bouton Wi-Fi, ON/OFF, LED)
- un **watchdog externe** pour relancer la carte principale en cas de plantage

👉 Ce circuit imprimé est à souder sur la carte **TTGO SIM7080G**.

🌐 Un serveur web embarqué vous permet de calibrer vos balances et de configurer l’adresse et le port pour vos messages MQTT.


![PCB](https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/blob/main/Balance%20Nb_iot/images/pcb.jpg)
![TTGO](https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/blob/main/Balance%20Nb_iot/images/TTGO_SIM7080G.jpg)

---

<a name="liste-des-composants-boitier-electronique"></a>
## 🧩 Liste des composants – Boîtier électronique

| Image | Composant | Description | Qté | Remarques |
|:--:|---|---|:--:|---|
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/TTGO_SIM7080G.jpg" width="80"/> | **Carte TTGO SIM7080G (LilyGO)** | Module principal NB-IoT + ESP32 | 1 | Communication + gestion énergie |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/pcb.jpg" width="80"/> | **PCB Beehive_Scale** | Interface capteurs de charge | 1 | Spécifique au projet |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Phoenix%201989777.png" width="80"/> | **Bornier Phoenix 1989777 (5 broches)** | Connexions capteurs | 2 | À souder sur le PCB |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Phoenix%201989816.png" width="80"/> | **Bornier Phoenix 1989816 (9 broches)** | Connexions capteurs | 1 | À souder sur le PCB |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/push%20button.png" width="80"/> | **Boutons poussoirs 12 mm** | Reset / calibration | 2 | Étanches IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/led.png" width="80"/> | **LED 6 mm** | Indication d’état | 1 | Couleur libre |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/DS18b20.png" width="80"/> | **Sonde DS18B20** | Température ambiante | 1 | Étanche |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vent.png" width="80"/> | **Soupape d’équilibrage** | Évite la condensation | 1 | IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/oring.png" width="80"/> | **Joint torique Ø2 mm** | Étanchéité couvercle | 1 | Silicone ou nitrile |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/panneau%20solaire.png" width="80"/> | **Panneau solaire 1W / 6V** | Alimentation solaire | 1 | ~100×80 mm |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/SP1310-SP1312%205%20pin.png" width="80"/> | **Connecteurs étanches 5 pins** | Connexion balance | Plusieurs | IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/SP1310-SP1312%202%20pin.png" width="80"/> | **Connecteurs étanches 2 pins** | Connexion solaire | Plusieurs | IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/insert.png" width="80"/> | **Inserts laiton M3** | Fixation boîtier 3D | 6 | À insérer à chaud |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/o-lube.png" width="80"/> | **Graisse silicone** | Lubrification joints | - | Améliore l’étanchéité |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vis%20TTGO.png" width="80"/> | **Vis de fixation TTGO** | Fixation carte | 4 | Inox |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/ipx.png" width="80"/> | **Câble IPX → SMA-K (5 cm)** | Liaison antenne | 1 | Pour module SIM7080G |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Antenna.png" width="80"/> | **Antenne NB-IoT / 4G** | Transmission réseau | 1 | SMA externe |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/cable.png" width="80"/> | **Câble 2 brins (2 m)** | Connexion solaire | 1 | Étanche |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/aimants.png" width="80"/> | **Aimants 12×3 mm** | Fixation panneau solaire | 2 | Néodyme |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/PG7.png" width="80"/> | **Presse-étoupe PG7 (12 mm)** | Passage câble | 1 | IP68 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/18650.png" width="80"/> | **Batterie Li-Ion 18650 (3000 mAh)** | Alimentation interne | 1 |  |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/gaine%20thermo.png" width="80"/> | **Gaine thermorétractable** | Protection connexions | - | Étanchéité |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/fil%2026%20awg.png" width="80"/> | **Fils AWG26** | Câblage interne | - | LED, boutons |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/micro%20sd.jpg" width="80"/> | **Carte microSD 16 Go** | Stockage local | 1 | FAT32 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/sim.png" width="80"/> | **Carte SIM NB-IoT** | Accès réseau | 1 | M2M ou prépayée |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/boîtier%20TTGO.png" width="80"/> | **Boîtier imprimé 3D (PETG)** | Conteneur principal | 1 | Étanche et modulaire |

---

<a name="liste-des-composants-structure-de-la-balance"></a>
## ⚖️ Liste des composants – Structure de la balance

| Image | Composant | Description | Qté | Remarques |
|:--:|---|---|:--:|---|
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Na4%20Mavin.png" width="80"/> | **Capteurs de charge NA4 Mavin** | Mesure du poids | 2 | 100 ou 200 kg |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/profilé%2030x60.png" width="80"/> | **Profilé alu 30×60 Type B** | Structure principale | 2 | 480 mm |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/profilé%2030x30.png" width="80"/> | **Profilé alu 30×30 Type B** | Structure secondaire | 4 | 430 mm |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/equerre%20interne.png" width="80"/> | **Équerres internes** | Assemblage mécanique | 8 | Rainure 8 M6 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/tasseau%20lourd.png" width="80"/> | **Tasseaux lourds B-Type M5** | Fixation structurelle | 2 | Maintien |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/D912.png" width="80"/> | **Visserie DIN 912** | M8x16 / M8x45 / M5x8 | - | Inox |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/drill.png" width="80"/> | **Foret étagé M8 (9–14 mm)** | Perçage profilés | 1 | |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/taraud.png" width="80"/> | **Taraud M8** | Filetage profilés | 1 | Montage précis |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vis%20boitier.png" width="80"/> | **Vis de boîtier** | Fixation boîtier | 4 | Inox |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/3dkit.png" width="80"/> | **Kit impression 3D (PETG)** | Pièces structurelles | 1 | Supports, entretoises |

---

<a name="estimation-des-couts-5-stations-et-10-balances"></a>
## 💰 Estimation des coûts — 5 stations et 10 balances (janvier 2026)

> ⚠️ Estimation indicative : les prix varient selon fournisseurs, quantités et frais de port.

### Balances (x10)

#### Motedis.fr

| Élément | Coût |
|---|---:|
| Kit Motedis | 465 € TTC |

#### Aliexpress

| Élément | Coût |
|---|---:|
| Connecteurs SP13 2 pins | 11 € |
| Connecteurs SP13 2 pins | 28 € |
| Peson 200 kg | 250 € |
| **Sous-total Balances** | **754 €** |

---

### Stations (x5)

#### Aliexpress

| Élément | Coût |
|---|---:|
| Carte TTGO SIM7080G | 200 € |
| Sonde température DS18B20 | 4,3 € |
| Câble IPX | 2,5 € |
| Antenne 4G | 4,5 € |
| Câble panneau solaire | 7 € |
| Aimant | 4,2 € |
| Reniflard | 4 € |
| Bouton poussoir | 5 € |
| Vis boîtier | 1,85 € |
| Vis TTGO | 1,8 € |
| LED rouge 3V | 6 € |
| Joint torique 2 mm × 5 m | 3,3 € |
| Câble 28 AWG | 1 € |
| Gaine thermo | 1 € |
| Colliers | 0,5 € |
| Inserts laiton | 1,8 € |

#### Mouser

| Élément | Coût |
|---|---:|
| Panneau solaire 1W | 20,5 € |
| Connecteurs 5 pins | 8 € |
| Connecteurs 12 pins | 9,7 € |
| FDP | 20 € *(ou 0 € si commande > 50 €)* |

#### JLCPCB

| Élément | Coût |
|---|---:|
| PCB | 54 € |
| *(108 € pour 10 cartes)* | |

#### Divers

| Élément | Coût |
|---|---:|
| 5 SIM NB-IoT Bouygues Telecom | 90 € |
| Impression 3D PETG | 40 € |
| Vernis de tropicalisation | 5 € |

---

### Total

| Total | Coût |
|---|---:|
| **Total global** | **1230 €** |
| **Coût moyen (par station)** | **246 €** |

---

<a name="etapes-dassemblage"></a>
## ⚙️ Étapes d’assemblage

*A faire.*

---

<a name="telechargement"></a>
## 📥 Téléchargements

### 🔌 Fichiers PCB

- **Dossier PCB (schémas + gerbers + BOM)**  
  👉 https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/tree/main/Balance%20Nb_iot/PCB

### 🧩 Fichiers STL pour impression 3D

- **Dossier STL**  
  👉 https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/tree/main/Balance%20Nb_iot/Fichiers%20stl

---

<a name="firmware"></a>
## 💾 Firmware

- **Dossier Firmware**  
  👉 https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/tree/main/Balance%20Nb_iot/Firmware

---

<a name="mode-demploi"></a>
## 📘 Mode d'emploi

Pour consulter le **mode d'emploi complet** de la Balance Connectée NB-IoT, rendez-vous ici :  
👉 [Mode d'emploi](https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/tree/main/Balance%20Nb_iot/Mode%20d'emploi)


<a name="web-iot"></a>
## 🌐 Partie Web et IoT

La **balance connectée** communique via un **serveur web embarqué** et envoie ses données au format **MQTT** vers un **broker Mosquitto**.  

Les données envoyées par MQTT comprennent :

- ⚖️ **Poids** : mesures des deux balances  
- 🌡️ **Température** : température ambiante mesurée par la sonde DS18B20  
- 🔋 **Batterie** : pourcentage de charge de la batterie interne  
- 📶 **Signal NB-IoT** : force du signal réseau  
- 📍 **Position** : coordonnées GPS exactes et position approximative (mise à jour 1 fois par jour)



Voici le flux des données :  

- 📨 **Envoi des mesures** : la balance publie ses données en MQTT.  
- ⚙️ **Traitement** : les messages peuvent être formatés et transformés par **Telegraf**.  
- 🗄️ **Stockage** : les données sont enregistrées dans une **base InfluxDB v2**.  
- 📊 **Visualisation** : Grafana permet de suivre les mesures en **temps réel** et d’afficher les historiques.  

> 💡 Exemple : les fichiers MQTT peuvent être traités comme vous le souhaitez, selon vos besoins.  

Toutes les données peuvent être :  

- 🖥️ **Stockées localement** sur un serveur dédié.  
- ☁️ Ou hébergées **dans le cloud** via un prestataire pour un accès distant et sécurisé.  

Cette architecture flexible permet d’**adapter le système** à différents besoins, tout en gardant la possibilité d’une **supervision centralisée ou locale**.



<a name="licence"></a>
## 📜 Licence

Ce projet est distribué sous licence **MIT**.  
Vous êtes libres de l’utiliser, le modifier et le partager, tant que la licence originale est conservée.

---

<a name="auteur"></a>
## 👤 Auteur

**Auteur :** [Ratamuse](https://github.com/Ratamuse)  
**Projet :** Balance Connectée NB-IoT  
**Version :** v1.0  
**Année :** 2026
