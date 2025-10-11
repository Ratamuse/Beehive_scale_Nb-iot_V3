# ⚖️ Balance Connectée NB-IoT

Ce projet décrit la construction d’un **boîtier électronique** pour une **balance connectée NB-IoT**, basée sur la carte **TTGO7080G** de LilyGO.  
L’objectif est de fournir un système **autonome, étanche et solaire**, capable de transmettre les données de **pesée** et de **température** via le réseau **NB-IoT**.

---

## 🧩 Liste des composants – Boîtier électronique

| Image | Composant | Description | Quantité | Remarques |
|:------:|------------|--------------|-----------|------------|
| <img src="images/TTGO_SIM7080G.jpg" width="80"/> | **Carte TTGO7080G (LilyGO)** | Module principal NB-IoT + microcontrôleur ESP32 | 1 | Gère la communication et la logique du système |
| <img src="images/pcb.jpg" width="80"/> | **Plaquette Beehive_Scale** | Interface de mesure de charge (capteur de poids) | 1 | Spécifique au projet balance |
| <img src="images/push%20button.png" width="80"/> | **Boutons poussoirs 12 mm** | Commandes manuelles (reset, calibration) | 2 | Étanches de préférence |
| <img src="images/led.png" width="80"/> | **LED 6 mm** | Indication d’état (alimentation, transmission, erreur) | 1 | Couleur au choix |
| <img src="images/DS18b20.png" width="80"/> | **Sonde de température DS18B20** | Mesure la température ambiante | 1 | Version étanche recommandée |
| <img src="images/vent.png" width="80"/> | **Soupape** | Équilibrage de pression du boîtier | 1 | Permet d’éviter la condensation |
| <img src="images/oring.png" width="80"/> | **Joint torique 2 mm** | Étanchéité du couvercle | 1 | Silicone ou nitrile selon usage |
| <img src="images/panneau%20solaire.png" width="80"/> | **Panneau solaire 1W / 6V max** | Alimentation solaire autonome | 1 | Seeed Studio 100×80 mm |
| <img src="images/SP1310-SP1312%205%20pin.png" width="80"/> | **Connecteurs étanches 5 pins** | Connexion pour la balance | Plusieurs | Type IP67 ou supérieur |
| <img src="images/SP1310-SP1312%202%20pin.png" width="80"/> | **Connecteurs étanches 2 pins** | Connexion panneau solaire | Plusieurs | Type IP67 ou supérieur |
| <img src="images/insert.png" width="80"/> | **Inserts laiton M3** | Fixation du boîtier 3D | 6 | À insérer à chaud |
| <img src="images/o-lube.png" width="80"/> | **Graisse silicone** | Lubrification des joints | - | Améliore l’étanchéité |
| <img src="images/vis%20TTGO.png" width="80"/> | **Vis de fixation TTGO** | Fixation de la carte TTGO | 4 | Acier inoxydable |
| <img src="images/ipx.png" width="80"/> | **Câble IPX → SMA-K (5 cm)** | Connexion antenne NB-IoT | 1 | Pour module SIM7080G |
| <img src="images/Antenna.png" width="80"/> | **Antenne NB-IoT** | Transmission des données | 1 | Compatible SIM7080G |
| <img src="images/cable.png" width="80"/> | **Câble 2 brins (2 m)** | Connexion panneau solaire | 1 | Câble souple et étanche |
| <img src="images/aimants.png" width="80"/> | **Aimants 12×3 mm** | Fixation du support solaire | 2 | Néodyme recommandé |
| <img src="images/PG7.png" width="80"/> | **Presse-étoupe PG7 (12 mm)** | Passage de câble étanche | 1 | Type IP68 recommandé |
| <img src="images/18650.png" width="80"/> | **Batterie Li-Ion 18650 (3000 mAh)** | Alimentation interne | 1 | Avec protection BMS intégrée |
| <img src="images/gaine%20thermo.png" width="80"/> | **Gaine thermorétractable** | Protection des connexions | - | Utilisée sur câbles et connecteurs |
| <img src="images/fil%2026%20awg.png" width="80"/> | **Fils AWG 26** | Câblage interne | - | Pour boutons et LED |
| <img src="images/TTGO.png" width="80"/> | **Carte SD 16 Go** | Enregistrement local des données | 1 | Format FAT32 recommandé |
| <img src="images/SIM.png" width="80"/> | **Carte SIM NB-IoT** | Abonnement réseau | 1 | Prépayée ou M2M |

---

## ⚖️ Liste des composants – Structure de la balance  
*(Deux balances par boîtier électronique)*

| Image | Composant | Description | Quantité | Remarques |
|:------:|------------|--------------|-----------|------------|
| <img src="images/Na4%20Mavin.png" width="80"/> | **Capteur de charge NA4 Mavin** | Mesure du poids | 2 | 100 kg ou 200 kg selon modèle |
| <img src="images/profilé%2030x60.png" width="80"/> | **Profilé aluminium 30×60 Type B** | Structure principale | 2 | Longueur : 480 mm |
| <img src="images/profilé%2030x30.png" width="80"/> | **Profilé aluminium 30×30 Type B** | Structure secondaire | 4 | Longueur : 430 mm |
| <img src="images/equerre%20interne.png" width="80"/> | **Équerre interne en zamak** | Assemblage mécanique | 8 | Rainure 8 M6 |
| <img src="images/tasseau%20lourd.png" width="80"/> | **Tasseau lourd rainure 8 B-Type M5** | Fixation interne | 2 | Maintien structurel |
| <img src="images/D912.png" width="80"/> | **Visserie DIN 912 M8x16 / M8x45 / M5x8** | Vis inoxydable | - | Selon montage |
| <img src="images/drill.png" width="80"/> | **Foret étagé M8 (9 et 14 mm)** | Perçage des profilés | 1 | Pour passage des vis |
| <img src="images/taraud.png" width="80"/> | **Taraud M8** | Filetage des trous percés | 1 | Pour montage précis |
| <img src="images/vis%20boitier.png" width="80"/> | **Vis de boîtier** | Fixation du boîtier à la balance | 4 | Acier inoxydable |
| <img src="images/3dkit.png" width="80"/> | **Kit impression 3D (PETG)** | Pièces structurelles | 1 | Support, couvercle, entretoises |

---

## ⚙️ Assemblage

1. Fixer la **carte TTGO7080G** et la **plaquette Beehive_Scale** dans le boîtier 3D.  
2. Câbler les **boutons**, la **LED** et la **sonde DS18B20** selon le schéma.  
3. Poser le **joint torique** et la **soupape** pour assurer l’étanchéité.  
4. Connecter le **panneau solaire** et la **batterie 18650** au module de charge TTGO.  
5. Vérifier toutes les connexions au multimètre avant fermeture.  
6. Fermer le boîtier, tester la communication NB-IoT et la calibration.  
7. Fixer les **capteurs NA4 Mavin** sur la structure aluminium.  
8. Effectuer la **calibration finale** à vide et en charge connue.

---

## 🔋 Alimentation & Autonomie

- **Source principale :** Panneau solaire 1 W / 6 V  
- **Stockage énergie :** Batterie Li-Ion 18650 3000 mAh  
- **Gestion de charge :** Intégrée (carte TTGO SIM7080G)  
- **Autonomie estimée :** Jusqu’à 3 semaines sans soleil

---

## 📡 Connectivité

- **Réseau :** NB-IoT  
- **Module :** SIM7080G intégré à la TTGO  
- **Carte SIM :** NB-IoT (M2M, LTE Cat NB1)  
- **Antenne :** SMA externe (fourni ou optionnelle)

---

## 🖨️ Fichiers 3D

- [📁 Boîtier 3D (STL)](./3d/boitier_balance.stl) *(à venir)*  
- [📁 Support capteur de charge](./3d/support_capteur.stl) *(optionnel)*

---

## 🧠 À venir

- 🔌 Schéma électronique détaillé  
- 💻 Code source du firmware (ESP32 + NB-IoT)  
- ⚖️ Guide de calibration complet  
- ☁️ Intégration Cloud (API NB-IoT → serveur)

---

## 📜 Licence

Ce projet est distribué sous licence **MIT**.  
Vous êtes libres de l’utiliser, le modifier et le partager, tant que la licence originale est conservée.

---

**Auteur :** [Ratamuse](https://github.com/Ratamuse)  
**Projet :** Balance Connectée NB-IoT  
**Version :** v1.0
