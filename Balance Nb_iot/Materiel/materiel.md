# ⚖️ Balance Connectée NB-IoT  
### Système de pesée autonome, étanche et solaire basé sur TTGO SIM7080G

![GitHub Repo](https://img.shields.io/badge/GitHub-Ratamuse--Beehive__scale__Nb--iot__V3-blue?logo=github)
![Langue](https://img.shields.io/badge/Langue-Français-blue)
![Version](https://img.shields.io/badge/Version-v1.0-green)
![Licence](https://img.shields.io/badge/Licence-MIT-lightgrey)
![Auteur](https://img.shields.io/badge/Auteur-Ratamuse-orange)

---

## 📚 Table des matières
1. [Présentation du projet](#-présentation-du-projet)
2. [Composants — Boîtier électronique](#-liste-des-composants--boîtier-électronique)
3. [Composants — Structure de la balance](#-liste-des-composants--structure-de-la-balance)
4. [Assemblage](#-étapes-dassemblage)
5. [Alimentation & Autonomie](#-alimentation--autonomie)
6. [Connectivité](#-connectivité)
7. [Fichiers 3D](#-fichiers-3d)
8. [Évolutions prévues](#-évolutions-prévues)
9. [Licence](#-licence)
10. [Auteur](#-auteur)

---

## 🧠 Présentation du projet

Ce projet décrit la conception d’un **boîtier électronique intelligent** pour une **balance connectée NB-IoT**, basée sur la carte **TTGO SIM7080G (LilyGO)**.  
L’objectif est de proposer une solution **autonome, robuste et étanche**, capable de **mesurer le poids et la température**, puis de **transmettre les données via le réseau NB-IoT**.

Le système est alimenté par **panneau solaire** et **batterie Li-Ion**, et conçu pour une installation extérieure (ex. : ruche, silo, plateforme logistique).

---

## 🧩 Liste des composants – Boîtier électronique

| Image | Composant | Description | Qté | Remarques |
|:------:|------------|--------------|:--:|------------|
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/TTGO_SIM7080G.jpg" width="80"/> | **Carte TTGO SIM7080G (LilyGO)** | Module principal NB-IoT + ESP32 | 1 | Communication, gestion d’énergie |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/pcb.jpg" width="80"/> | **PCB Beehive_Scale** | Interface capteur de charge | 1 | Spécifique au projet |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Phoenix%201989777.png" width="80"/> | **Bornier 5 broches Phoenix 1989777** | Connexions capteurs | 2 | À souder sur la plaquette |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Phoenix%201989816.png" width="80"/> | **Bornier 9 broches Phoenix 1989816** | Connexions capteurs | 1 | À souder |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/push%20button.png" width="80"/> | **Boutons poussoirs 12 mm** | Reset / Calibration | 2 | Étanches IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/led.png" width="80"/> | **LED 6 mm** | Indication d’état | 1 | Couleur libre |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/DS18b20.png" width="80"/> | **Sonde DS18B20** | Température ambiante | 1 | Étanche |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vent.png" width="80"/> | **Soupape d’équilibrage** | Évite la condensation | 1 | IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/oring.png" width="80"/> | **Joint torique Ø2 mm** | Étanchéité couvercle | 1 | Silicone ou nitrile |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/panneau%20solaire.png" width="80"/> | **Panneau solaire 1W / 6V** | Alimentation solaire | 1 | 100×80 mm |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/SP1310-SP1312%205%20pin.png" width="80"/> | **Connecteurs étanches 5 pins** | Connexion balance | Plusieurs | IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/SP1310-SP1312%202%20pin.png" width="80"/> | **Connecteurs étanches 2 pins** | Connexion solaire | Plusieurs | IP67 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/insert.png" width="80"/> | **Inserts laiton M3** | Fixation boîtier 3D | 6 | À insérer à chaud |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/o-lube.png" width="80"/> | **Graisse silicone** | Lubrification joints | - | Améliore étanchéité |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vis%20TTGO.png" width="80"/> | **Vis de fixation TTGO** | Fixation carte | 4 | Inox |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/ipx.png" width="80"/> | **Câble IPX → SMA-K (5 cm)** | Liaison antenne | 1 | Pour module SIM7080G |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Antenna.png" width="80"/> | **Antenne NB-IoT** | Transmission réseau | 1 | SMA externe |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/cable.png" width="80"/> | **Câble 2 brins (2 m)** | Connexion solaire | 1 | Étanche |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/aimants.png" width="80"/> | **Aimants 12×3 mm** | Fixation panneau solaire | 2 | Néodyme |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/PG7.png" width="80"/> | **Presse-étoupe PG7 (12 mm)** | Passage câble | 1 | IP68 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/18650.png" width="80"/> | **Batterie Li-Ion 18650 (3000 mAh)** | Alimentation interne | 1 | Avec BMS |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/gaine%20thermo.png" width="80"/> | **Gaine thermorétractable** | Protection connexions | - | Étanchéité |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/fil%2026%20awg.png" width="80"/> | **Fils AWG 26** | Câblage interne | - | Pour LED, boutons |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/micro%20sd.jpg" width="80"/> | **Carte microSD 16 Go** | Stockage local | 1 | FAT32 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/sim.png" width="80"/> | **Carte SIM NB-IoT** | Accès réseau | 1 | M2M ou prépayée |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/boîtier%20TTGO.png" width="80"/> | **Boîtier imprimé 3D (PETG)** | Conteneur principal | 1 | Étanche et modulaire |

---

## ⚖️ Liste des composants – Structure de la balance

| Image | Composant | Description | Qté | Remarques |
|:------:|------------|--------------|:--:|------------|
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

## ⚙️ Étapes d’assemblage

1. **Montage électronique :**  
   - Fixer la carte **TTGO** et la **Beehive_Scale** dans le boîtier.  
   - Câbler les boutons, LED et sonde DS18B20.  

2. **Étanchéité :**  
   - Poser le **joint torique** et la **soupape**.  
   - Appliquer la **graisse silicone**.  

3. **Tests :**  
   - Vérifier tension, polarité, communication NB-IoT.  

4. **Montage final :**  
   - Fermer le boîtier.  
   - Installer les **capteurs de charge** sur la structure aluminium.  
   - Effectuer la **calibration à vide et en charge connue**.

---

## 🔋 Alimentation & Autonomie

| Élément | Spécifications |
|----------|----------------|
| **Source principale** | Panneau solaire 1 W / 6 V |
| **Stockage énergie** | Batterie Li-Ion 18650 (3000 mAh) |
| **Gestion de charge** | Intégrée sur la carte TTGO |
| **Autonomie estimée** | Jusqu’à 3 semaines sans soleil |

---

## 📡 Connectivité

| Élément | Spécifications |
|----------|----------------|
| **Réseau** | NB-IoT (LTE Cat NB1) |
| **Module** | SIM7080G intégré à la TTGO |
| **Antenne** | SMA externe |
| **Carte SIM** | M2M ou prépayée |

---

## 🖨️ Fichiers 3D

- [📁 Boîtier principal (STL)](./3d/boitier_balance.stl) *(à venir)*  
- [📁 Support capteur (STL)](./3d/support_capteur.stl) *(optionnel)*  

---

## 🚀 Évolutions prévues

- 🔌 Schéma électronique détaillé  
- 💻 Code firmware ESP32 + NB-IoT  
- ⚖️ Guide de calibration complet  
- ☁️ Intégration Cloud (API → serveur)

---

## 📜 Licence

Ce projet est distribué sous licence **MIT**.  
Vous êtes libres de l’utiliser, le modifier et le partager, tant que la licence originale est conservée.

---

## 👤 Auteur

**Auteur :** [Ratamuse](https://github.com/Ratamuse)  
**Projet :** Balance Connectée NB-IoT  
**Version :** v1.0  
**Année :** 2025

---

