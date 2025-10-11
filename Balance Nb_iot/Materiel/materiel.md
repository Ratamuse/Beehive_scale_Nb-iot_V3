# ⚖️ Balance Connectée NB-IoT

Ce projet décrit la construction d’un **boîtier électronique** pour une **balance connectée NB-IoT**, basée sur la carte **TTGO7080G** de LilyGO.  
L’objectif est de fournir un système autonome et étanche, capable de transmettre les données de pesée et de température via le réseau NB-IoT.

---

## 🧩 Liste des composants pour le boîtier électronique

| Image | Composant | Description | Quantité | Remarques |
|:------:|------------|--------------|-----------|------------|
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/TTGO_SIM7080G.jpg" width="80"/> | **Carte TTGO7080G (LilyGO)** | Module principal NB-IoT + microcontrôleur ESP32 | 1 | Gère la communication et la logique du système |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/pcb.jpg" width="80"/> | **Plaquette Beehive_Scale** | Interface de mesure de charge (capteur de poids) | 1 | Spécifique au projet balance |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/push%20button.png" width="80"/> | **Boutons poussoirs 12 mm** | Commandes manuelles (ex. reset, calibration) | 2 | Étanches de préférence |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/led.png" width="80"/> | **LED 6 mm** | Indication d’état (alimentation, transmission, erreur) | 1 | Couleur au choix |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/DS18b20.png" width="80"/> | **Sonde de température DS18B20** | Mesure la température ambiante | 1 | Version étanche recommandée |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vent.png" width="80"/> | **Soupape** | Équilibrage de pression du boîtier | 1 | Permet d’éviter la condensation |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/oring.png" width="80"/> | **Joint torique 2 mm** | Étanchéité du couvercle | 1 | Silicone ou nitrile selon usage |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/panneau%20solaire.png" width="80"/> | **Panneau solaire 1W / 6V max** | Alimentation autonome via batterie | 1 | Compatible avec TTGO7080G |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/SP1310-SP1312%205%20pin.png" width="80"/> | **Connecteurs étanches 5 pins** | Connexions pour la balance | Plusieurs | Type IP67 ou supérieur recommandé |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/SP1310-SP1312%202%20pin.png" width="80"/> | **Connecteurs étanches 2 pins** | Connexions pour panneau solaire | Plusieurs | Type IP67 ou supérieur recommandé |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/insert.png" width="80"/> | **Inserts laiton** | Fixation du boîtier 3D | 6 | À insérer à chaud |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/o-lube.png" width="80"/> | **Graisse silicone** | Lubrification des joints toriques | - | Améliore l’étanchéité |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vis%20TTGO.png" width="80"/> | **Vis de fixation TTGO** | Fixation de la carte TTGO | 4 | Acier inoxydable |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/ipx.png" width="80"/> | **Câble IPX SMA-K (5 cm)** | Connexion antenne NB-IoT | 1 | Pour module SIM7080G |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Antenna.png" width="80"/> | **Antenne NB-IoT** | Transmission des données | 1 | Compatible SIM7080G |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/cable.png" width="80"/> | **Câble 2 brins (2 m)** | Connexion du panneau solaire | 1 | Câble souple étanche |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/aimants.png" width="80"/> | **Aimants 12x3 mm** | Fixation du support solaire | 2 | Néodyme recommandé |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/PG7.png" width="80"/> | **Presse-étoupe PG7 (12 mm)** | Passage de câble étanche | 1 | Type IP68 recommandé |

---

## ⚖️ Liste des composants pour une balance (deux par boîtier électronique)

| Image | Composant | Description | Quantité | Remarques |
|:------:|------------|--------------|-----------|------------|
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/Na4%20Mavin.png" width="80"/> | **Capteur de charge NA4 Mavin** | Mesure du poids | 2 | Un capteur par côté de balance |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/profilé%2030x60.png" width="80"/> | **Profilé aluminium 30x60 Type B** | Structure principale | 2 | 480 mm |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/profilé%2030x30.png" width="80"/> | **Profilé aluminium 30x30 Type B** | Structure secondaire | 4 | 430 mm |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/equerre%20interne.png" width="80"/> | **Équerre interne en zamak** | Assemblage mécanique | 8 | Rainure 8 M6 |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/tasseau%20lourd.png" width="80"/> | **Tasseau lourd rainure 8 B-Type M5** | Fixation interne | 2 | Pour maintien structurel |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/D912.png" width="80"/> | **Vis DIN 912 M8x16 / M8x45 / M5x8** | Visserie inox | - | Selon montage |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/drill.png" width="80"/> | **Foret étagé M8 (9 et 14 mm)** | Perçage des profilés | 1 | Pour passage des vis |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/taraud.png" width="80"/> | **Taraud M8** | Filetage des trous percés | 1 | Pour montage précis |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/vis%20boitier.png" width="80"/> | **Vis de boîtier** | Fixation du boîtier sur la balance | 4 | Acier inoxydable |
| <img src="https://github.com/Ratamuse/Beehive_scale_Nb-iot_V3/raw/main/Balance%20Nb_iot/images/3dkit.png" width="80"/> | **Kit impression 3D PETG** | Pièces structurelles | 1 | Support, couvercle, entretoises |

---

## ⚙️ Assemblage

1. Fixer la carte **TTGO7080G** et la **plaquette Beehive_Scale** dans le boîtier 3D.  
2. Relier les boutons poussoirs, la LED et la sonde DS18B20 aux broches prévues.  
3. Installer le **joint torique** et la **soupape** pour assurer l’étanchéité.  
4. Connecter le **panneau solaire** au module de charge du TTGO7080G.  
5. Vérifier les connexions à l’aide d’un multimètre avant fermeture.  
6. Fermer le boîtier et effectuer les tests de calibration.  
7. Fixer les capteurs sur la structure aluminium et calibrer la mesure.

---

## 🔋 Alimentation & Autonomie

- **Source principale :** Panneau solaire 1W / 6V  
- **Batterie interne :** Li-Ion (optionnelle selon la configuration TTGO)  
- **Gestion de charge intégrée :** Oui (selon la version de la carte)

---

## 📡 Connectivité

- **Réseau :** NB-IoT  
- **Module :** SIM7080G intégré à la TTGO  
- **Antennes :** Fournies ou à connecter via SMA  

---

## 🖨️ Fichiers 3D

- [📁 Boîtier 3D (STL)](./3d/boitier_balance.stl) *(à ajouter)*  
- [📁 Support capteur de charge](./3d/support_capteur.stl) *(optionnel)*

---

## 🧠 À venir

- Schéma électronique détaillé  
- Code source du firmware  
- Guide de calibration de la balance  
- Intégration cloud (API NB-IoT → serveur)

---

## 📜 Licence

Ce projet est distribué sous licence **MIT**.  
Vous êtes libres de l’utiliser, le modifier et le partager, tant que la licence originale est conservée.

---

**Auteur :** [Ratamuse](https://github.com/Ratamuse)  
**Projet :** Balance connectée NB-IoT  
**Version :** v1.0
