# ⚖️ Balance Connectée NB-IoT

Ce projet décrit la construction d’un **boîtier électronique** pour une **balance connectée NB-IoT**, basée sur la carte **TTGO7080G** de LilyGO.  
L’objectif est de fournir un système autonome et étanche, capable de transmettre les données de pesée et de température via le réseau NB-IoT.

---

## 🧩 Liste des composants

| Composant | Description | Quantité | Remarques |
|------------|--------------|-----------|------------|
| **Carte TTGO7080G (LilyGO)** | Module principal NB-IoT + microcontrôleur ESP32 | 1 | Gère la communication et la logique du système |
| **Plaquette Beehive_Scale** | Interface de mesure de charge (capteur de poids) | 1 | Spécifique au projet balance |
| **Boutons poussoirs 12 mm** | Commandes manuelles (ex. reset, calibration) | 2 | Étanches de préférence |
| **LED 6 mm** | Indication d’état (alimentation, transmission, erreur) | 1 | Couleur au choix |
| **Sonde de température DS18B20** | Mesure la température ambiante | 1 | Version étanche recommandée |
| **Soupape** | Équilibrage de pression du boîtier | 1 | Permet d’éviter la condensation |
| **Joint torique 2 mm** | Étanchéité du couvercle ou des connecteurs | 1 | Silicone ou nitrile selon usage |
| **Boîtier imprimé 3D** | Support et protection des composants | 1 | Fichier STL à venir |
| **Panneau solaire 1W / 6V max** | Alimentation autonome via batterie | 1 | Compatible avec TTGO7080G |
| **Connecteurs étanches** | Connexions pour capteurs et alimentation | Plusieurs | Type IP67 ou supérieur recommandé |

---

## ⚙️ Assemblage

1. Fixer la carte **TTGO7080G** et la **plaquette Beehive_Scale** dans le boîtier 3D.  
2. Relier les boutons poussoirs, la LED et la sonde DS18B20 aux broches prévues.  
3. Installer le **joint torique** et la **soupape** pour assurer l’étanchéité.  
4. Connecter le **panneau solaire** au module de charge du TTGO7080G.  
5. Vérifier les connexions à l’aide d’un multimètre avant fermeture.  
6. Fermer le boîtier et effectuer les tests de calibration.

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

**Auteur :** [TonNom]  
**Projet :** Balance connectée NB-IoT  
**Version :** v1.0  
