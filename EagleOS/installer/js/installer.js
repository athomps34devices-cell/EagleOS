"use strict";

const STORAGE_KEY = "eagleos_installer_settings";

const DEFAULT_SETTINGS = {
    language: "English",
    region: "United States",
    keyboard: "US",
    installationType: "clean",
    destination: "disk1",
    preferences: {
        recoveryPartition: true,
        automaticUpdates: true,
        diagnosticData: false,
        desktopShortcuts: true
    },
    preferencesCompleted: false
};

const LANGUAGE_NAMES = [
    "English",
    "Español",
    "Français",
    "Deutsch",
    "Português",
    "Italiano",
    "日本語",
    "한국어",
    "中文"
];

const REGIONS = [
    "United States",
    "United Kingdom",
    "Canada",
    "Australia",
    "New Zealand",
    "Germany",
    "France",
    "Spain",
    "Italy",
    "Portugal",
    "Brazil",
    "Japan",
    "South Korea",
    "China"
];

const KEYBOARD_LAYOUTS = [
    "US",
    "UK",
    "German",
    "French",
    "Japanese"
];

const SCREENS = [
    "welcome",
    "language",
    "license",
    "integrity",
    "region",
    "keyboard",
    "installation",
    "destination",
    "preferences",
    "summary",
    "installing",
    "complete"
];

const DISKS = [
    {
        id: "disk1",
        name: "Disk 0",
        model: "Primary Storage Drive",
        capacity: "512 GB",
        available: "476 GB",
        type: "SSD"
    },
    {
        id: "disk2",
        name: "Disk 1",
        model: "Secondary Storage Drive",
        capacity: "1 TB",
        available: "931 GB",
        type: "SSD"
    },
    {
        id: "disk3",
        name: "Disk 2",
        model: "External Storage Drive",
        capacity: "2 TB",
        available: "1.81 TB",
        type: "HDD"
    }
];

const TRANSLATIONS = {
    English: {
        welcome: "Welcome",
        language: "Language",
        license: "License",
        integrity: "Integrity",
        region: "Region",
        keyboard: "Keyboard",
        installation: "Installation",
        destination: "Destination",
        preferences: "Preferences",
        summary: "Summary",
        installing: "Installing",
        complete: "Complete",
        sidebar: "EagleOS Setup",
        sidebarStatus: "Installation",
        welcomeTitle: "Welcome to EagleOS",
        welcomeDescription: "Let's get your computer ready for EagleOS.",
        welcomeMessage: "This setup will guide you through installing EagleOS on your computer.",
        welcomeNote: "Before beginning, make sure you have backed up any important files.",
        languageTitle: "Choose your language",
        languageDescription: "Select the language you want to use during setup.",
        languageLabel: "Display language",
        licenseTitle: "EagleOS License Agreement",
        licenseDescription: "Please review the license agreement before continuing.",
        licenseAgreement: "I have read and agree to the license agreement.",
        integrityTitle: "System Integrity Check",
        integrityDescription: "EagleOS Setup will verify that its installation files have not been modified.",
        integrityChecking: "Checking installation files...",
        integrityPassed: "All installation files passed integrity verification.",
        integrityFailed: "Some installation files could not be verified.",
        integrityAgree: "I understand the risks and want to continue anyway.",
        integrityAbort: "Do not continue with installation.",
        regionTitle: "Select your region",
        regionDescription: "Choose your country or region.",
        regionLabel: "Country or region",
        keyboardTitle: "Choose your keyboard layout",
        keyboardDescription: "Select the keyboard layout connected to your computer.",
        keyboardLabel: "Keyboard layout",
        installationTitle: "Choose your installation type",
        installationDescription: "Select how you want EagleOS installed.",
        cleanInstallation: "Clean installation",
        cleanDescription: "Install a fresh copy of EagleOS. Existing data on the selected destination may be erased.",
        upgradeInstallation: "Upgrade installation",
        upgradeDescription: "Upgrade an existing EagleOS installation while keeping compatible files and settings.",
        destinationTitle: "Choose where to install EagleOS",
        destinationDescription: "Select the storage device you want to use.",
        destinationLabel: "Available storage devices",
        diskAvailable: "Available",
        diskCapacity: "Capacity",
        diskType: "Drive type",
        destinationWarning: "Make sure you select the correct storage device before continuing.",
        preferencesTitle: "Customize your installation",
        preferencesDescription: "Choose the features you want enabled in EagleOS.",
        recoveryPartition: "Create a recovery partition",
        recoveryPartitionDescription: "Reserve space for system recovery tools.",
        automaticUpdates: "Enable automatic updates",
        automaticUpdatesDescription: "Automatically download and install system updates.",
        diagnosticData: "Share diagnostic information",
        diagnosticDataDescription: "Help improve EagleOS by sharing optional diagnostic information.",
        desktopShortcuts: "Create desktop shortcuts",
        desktopShortcutsDescription: "Add useful shortcuts to your desktop.",
        summaryTitle: "Ready to install EagleOS",
        summaryDescription: "Review your installation choices before continuing.",
        summaryLanguage: "Language",
        summaryRegion: "Region",
        summaryKeyboard: "Keyboard",
        summaryInstallation: "Installation type",
        summaryDestination: "Destination",
        summaryPreferences: "Preferences",
        clean: "Clean installation",
        upgrade: "Upgrade installation",
        summaryWarning: "Please review your choices. You may not be able to undo changes made during installation.",
        installingTitle: "Installing EagleOS",
        installingDescription: "Please wait while EagleOS is installed.",
        installingDoNotTurnOff: "Do not turn off your computer during installation.",
        installingPreparing: "Preparing installation...",
        installingFiles: "Installing system files...",
        installingConfiguration: "Configuring EagleOS...",
        installingFinishing: "Finishing installation...",
        installingProgress: "Installation progress",
        completeTitle: "EagleOS is ready!",
        completeDescription: "The installation process has finished.",
        completeMessage: "Thank you for choosing EagleOS.",
        completeNote: "Your computer is ready for its next step.",
        completeButton: "Finish",
        back: "Back",
        next: "Next",
        cancel: "Cancel",
        finish: "Finish",
        footer: "EagleOS Setup",
        yes: "Yes",
        no: "No",
        enabled: "Enabled",
        disabled: "Disabled"
    },

    Español: {
        welcome: "Bienvenido",
        language: "Idioma",
        license: "Licencia",
        integrity: "Integridad",
        region: "Región",
        keyboard: "Teclado",
        installation: "Instalación",
        destination: "Destino",
        preferences: "Preferencias",
        summary: "Resumen",
        installing: "Instalando",
        complete: "Completado",
        sidebar: "Instalación de EagleOS",
        sidebarStatus: "Instalación",
        welcomeTitle: "Bienvenido a EagleOS",
        welcomeDescription: "Preparemos tu computadora para EagleOS.",
        welcomeMessage: "Esta instalación te guiará durante el proceso de instalación de EagleOS.",
        welcomeNote: "Antes de comenzar, asegúrate de haber guardado tus archivos importantes.",
        languageTitle: "Elige tu idioma",
        languageDescription: "Selecciona el idioma que deseas utilizar durante la instalación.",
        languageLabel: "Idioma de visualización",
        licenseTitle: "Acuerdo de licencia de EagleOS",
        licenseDescription: "Revisa el acuerdo de licencia antes de continuar.",
        licenseAgreement: "He leído y acepto el acuerdo de licencia.",
        integrityTitle: "Verificación de integridad del sistema",
        integrityDescription: "EagleOS Setup verificará que los archivos de instalación no hayan sido modificados.",
        integrityChecking: "Verificando archivos de instalación...",
        integrityPassed: "Todos los archivos de instalación superaron la verificación.",
        integrityFailed: "No se pudieron verificar algunos archivos de instalación.",
        integrityAgree: "Entiendo los riesgos y quiero continuar de todos modos.",
        integrityAbort: "No continuar con la instalación.",
        regionTitle: "Selecciona tu región",
        regionDescription: "Elige tu país o región.",
        regionLabel: "País o región",
        keyboardTitle: "Elige tu distribución de teclado",
        keyboardDescription: "Selecciona la distribución del teclado conectado a tu computadora.",
        keyboardLabel: "Distribución del teclado",
        installationTitle: "Elige el tipo de instalación",
        installationDescription: "Selecciona cómo deseas instalar EagleOS.",
        cleanInstallation: "Instalación limpia",
        cleanDescription: "Instala una copia nueva de EagleOS. Los datos existentes en el destino seleccionado podrían eliminarse.",
        upgradeInstallation: "Actualizar instalación",
        upgradeDescription: "Actualiza una instalación existente de EagleOS conservando los archivos y configuraciones compatibles.",
        destinationTitle: "Elige dónde instalar EagleOS",
        destinationDescription: "Selecciona el dispositivo de almacenamiento que deseas utilizar.",
        destinationLabel: "Dispositivos de almacenamiento disponibles",
        diskAvailable: "Disponible",
        diskCapacity: "Capacidad",
        diskType: "Tipo de unidad",
        destinationWarning: "Asegúrate de seleccionar el dispositivo correcto antes de continuar.",
        preferencesTitle: "Personaliza tu instalación",
        preferencesDescription: "Elige las funciones que deseas activar en EagleOS.",
        recoveryPartition: "Crear una partición de recuperación",
        recoveryPartitionDescription: "Reservar espacio para las herramientas de recuperación del sistema.",
        automaticUpdates: "Activar actualizaciones automáticas",
        automaticUpdatesDescription: "Descargar e instalar automáticamente las actualizaciones del sistema.",
        diagnosticData: "Compartir información de diagnóstico",
        diagnosticDataDescription: "Ayuda a mejorar EagleOS compartiendo información de diagnóstico opcional.",
        desktopShortcuts: "Crear accesos directos en el escritorio",
        desktopShortcutsDescription: "Agregar accesos directos útiles a tu escritorio.",
        summaryTitle: "Listo para instalar EagleOS",
        summaryDescription: "Revisa tus opciones de instalación antes de continuar.",
        summaryLanguage: "Idioma",
        summaryRegion: "Región",
        summaryKeyboard: "Teclado",
        summaryInstallation: "Tipo de instalación",
        summaryDestination: "Destino",
        summaryPreferences: "Preferencias",
        clean: "Instalación limpia",
        upgrade: "Actualizar instalación",
        summaryWarning: "Revisa tus opciones. Es posible que no puedas deshacer los cambios realizados durante la instalación.",
        installingTitle: "Instalando EagleOS",
        installingDescription: "Espera mientras se instala EagleOS.",
        installingDoNotTurnOff: "No apagues tu computadora durante la instalación.",
        installingPreparing: "Preparando la instalación...",
        installingFiles: "Instalando archivos del sistema...",
        installingConfiguration: "Configurando EagleOS...",
        installingFinishing: "Finalizando la instalación...",
        installingProgress: "Progreso de instalación",
        completeTitle: "¡EagleOS está listo!",
        completeDescription: "El proceso de instalación ha finalizado.",
        completeMessage: "Gracias por elegir EagleOS.",
        completeNote: "Tu computadora está lista para el siguiente paso.",
        completeButton: "Finalizar",
        back: "Atrás",
        next: "Siguiente",
        cancel: "Cancelar",
        finish: "Finalizar",
        footer: "Instalación de EagleOS",
        yes: "Sí",
        no: "No",
        enabled: "Activado",
        disabled: "Desactivado"
    },

    Français: {
        welcome: "Bienvenue",
        language: "Langue",
        license: "Licence",
        integrity: "Intégrité",
        region: "Région",
        keyboard: "Clavier",
        installation: "Installation",
        destination: "Destination",
        preferences: "Préférences",
        summary: "Résumé",
        installing: "Installation",
        complete: "Terminé",
        sidebar: "Installation d'EagleOS",
        sidebarStatus: "Installation",
        welcomeTitle: "Bienvenue sur EagleOS",
        welcomeDescription: "Préparons votre ordinateur pour EagleOS.",
        welcomeMessage: "Ce programme vous guidera tout au long de l'installation d'EagleOS sur votre ordinateur.",
        welcomeNote: "Avant de commencer, assurez-vous d'avoir sauvegardé vos fichiers importants.",
        languageTitle: "Choisissez votre langue",
        languageDescription: "Sélectionnez la langue à utiliser pendant l'installation.",
        languageLabel: "Langue d'affichage",
        licenseTitle: "Contrat de licence EagleOS",
        licenseDescription: "Veuillez lire le contrat de licence avant de continuer.",
        licenseAgreement: "J'ai lu et j'accepte le contrat de licence.",
        integrityTitle: "Vérification de l'intégrité du système",
        integrityDescription: "Le programme d'installation vérifiera que les fichiers n'ont pas été modifiés.",
        integrityChecking: "Vérification des fichiers d'installation...",
        integrityPassed: "Tous les fichiers ont passé la vérification d'intégrité.",
        integrityFailed: "Certains fichiers d'installation n'ont pas pu être vérifiés.",
        integrityAgree: "Je comprends les risques et souhaite continuer.",
        integrityAbort: "Ne pas continuer l'installation.",
        regionTitle: "Sélectionnez votre région",
        regionDescription: "Choisissez votre pays ou votre région.",
        regionLabel: "Pays ou région",
        keyboardTitle: "Choisissez votre disposition de clavier",
        keyboardDescription: "Sélectionnez la disposition du clavier connecté à votre ordinateur.",
        keyboardLabel: "Disposition du clavier",
        installationTitle: "Choisissez votre type d'installation",
        installationDescription: "Sélectionnez la manière dont vous souhaitez installer EagleOS.",
        cleanInstallation: "Installation propre",
        cleanDescription: "Installez une nouvelle copie d'EagleOS. Les données existantes sur le disque sélectionné peuvent être supprimées.",
        upgradeInstallation: "Mise à niveau",
        upgradeDescription: "Mettez à niveau une installation existante d'EagleOS tout en conservant les fichiers et paramètres compatibles.",
        destinationTitle: "Choisissez où installer EagleOS",
        destinationDescription: "Sélectionnez le périphérique de stockage à utiliser.",
        destinationLabel: "Périphériques de stockage disponibles",
        diskAvailable: "Disponible",
        diskCapacity: "Capacité",
        diskType: "Type de disque",
        destinationWarning: "Assurez-vous de sélectionner le bon périphérique avant de continuer.",
        preferencesTitle: "Personnalisez votre installation",
        preferencesDescription: "Choisissez les fonctionnalités à activer dans EagleOS.",
        recoveryPartition: "Créer une partition de récupération",
        recoveryPartitionDescription: "Réserver de l'espace pour les outils de récupération du système.",
        automaticUpdates: "Activer les mises à jour automatiques",
        automaticUpdatesDescription: "Télécharger et installer automatiquement les mises à jour du système.",
        diagnosticData: "Partager les données de diagnostic",
        diagnosticDataDescription: "Contribuez à améliorer EagleOS en partageant des données de diagnostic facultatives.",
        desktopShortcuts: "Créer des raccourcis sur le bureau",
        desktopShortcutsDescription: "Ajouter des raccourcis utiles à votre bureau.",
        summaryTitle: "Prêt à installer EagleOS",
        summaryDescription: "Vérifiez vos choix avant de continuer.",
        summaryLanguage: "Langue",
        summaryRegion: "Région",
        summaryKeyboard: "Clavier",
        summaryInstallation: "Type d'installation",
        summaryDestination: "Destination",
        summaryPreferences: "Préférences",
        clean: "Installation propre",
        upgrade: "Mise à niveau",
        summaryWarning: "Vérifiez vos choix. Vous ne pourrez peut-être pas annuler les modifications effectuées pendant l'installation.",
        installingTitle: "Installation d'EagleOS",
        installingDescription: "Veuillez patienter pendant l'installation d'EagleOS.",
        installingDoNotTurnOff: "N'éteignez pas votre ordinateur pendant l'installation.",
        installingPreparing: "Préparation de l'installation...",
        installingFiles: "Installation des fichiers système...",
        installingConfiguration: "Configuration d'EagleOS...",
        installingFinishing: "Finalisation de l'installation...",
        installingProgress: "Progression de l'installation",
        completeTitle: "EagleOS est prêt !",
        completeDescription: "Le processus d'installation est terminé.",
        completeMessage: "Merci d'avoir choisi EagleOS.",
        completeNote: "Votre ordinateur est prêt pour la prochaine étape.",
        completeButton: "Terminer",
        back: "Retour",
        next: "Suivant",
        cancel: "Annuler",
        finish: "Terminer",
        footer: "Installation d'EagleOS",
        yes: "Oui",
        no: "Non",
        enabled: "Activé",
        disabled: "Désactivé"
    },

    Deutsch: {
        welcome: "Willkommen",
        language: "Sprache",
        license: "Lizenz",
        integrity: "Integrität",
        region: "Region",
        keyboard: "Tastatur",
        installation: "Installation",
        destination: "Ziel",
        preferences: "Einstellungen",
        summary: "Zusammenfassung",
        installing: "Installation läuft",
        complete: "Abgeschlossen",
        sidebar: "EagleOS-Installation",
        sidebarStatus: "Installation",
        welcomeTitle: "Willkommen bei EagleOS",
        welcomeDescription: "Bereiten wir Ihren Computer auf EagleOS vor.",
        welcomeMessage: "Dieses Setup führt Sie durch die Installation von EagleOS auf Ihrem Computer.",
        welcomeNote: "Stellen Sie vor Beginn sicher, dass Sie wichtige Dateien gesichert haben.",
        languageTitle: "Sprache auswählen",
        languageDescription: "Wählen Sie die Sprache für die Installation aus.",
        languageLabel: "Anzeigesprache",
        licenseTitle: "EagleOS-Lizenzvereinbarung",
        licenseDescription: "Bitte lesen Sie die Lizenzvereinbarung, bevor Sie fortfahren.",
        licenseAgreement: "Ich habe die Lizenzvereinbarung gelesen und stimme ihr zu.",
        integrityTitle: "Systemintegritätsprüfung",
        integrityDescription: "EagleOS Setup überprüft, ob die Installationsdateien verändert wurden.",
        integrityChecking: "Installationsdateien werden überprüft...",
        integrityPassed: "Alle Installationsdateien haben die Integritätsprüfung bestanden.",
        integrityFailed: "Einige Installationsdateien konnten nicht überprüft werden.",
        integrityAgree: "Ich verstehe die Risiken und möchte trotzdem fortfahren.",
        integrityAbort: "Installation nicht fortsetzen.",
        regionTitle: "Region auswählen",
        regionDescription: "Wählen Sie Ihr Land oder Ihre Region aus.",
        regionLabel: "Land oder Region",
        keyboardTitle: "Tastaturlayout auswählen",
        keyboardDescription: "Wählen Sie das Layout der angeschlossenen Tastatur aus.",
        keyboardLabel: "Tastaturlayout",
        installationTitle: "Installationstyp auswählen",
        installationDescription: "Wählen Sie aus, wie EagleOS installiert werden soll.",
        cleanInstallation: "Neuinstallation",
        cleanDescription: "Installieren Sie eine neue Kopie von EagleOS. Vorhandene Daten auf dem ausgewählten Laufwerk können gelöscht werden.",
        upgradeInstallation: "Upgrade-Installation",
        upgradeDescription: "Aktualisieren Sie eine bestehende EagleOS-Installation und behalten Sie kompatible Dateien und Einstellungen bei.",
        destinationTitle: "Installationsort auswählen",
        destinationDescription: "Wählen Sie das gewünschte Speichergerät aus.",
        destinationLabel: "Verfügbare Speichergeräte",
        diskAvailable: "Verfügbar",
        diskCapacity: "Kapazität",
        diskType: "Laufwerkstyp",
        destinationWarning: "Stellen Sie sicher, dass Sie das richtige Speichergerät auswählen.",
        preferencesTitle: "Installation anpassen",
        preferencesDescription: "Wählen Sie die Funktionen aus, die in EagleOS aktiviert werden sollen.",
        recoveryPartition: "Wiederherstellungspartition erstellen",
        recoveryPartitionDescription: "Speicherplatz für Systemwiederherstellungstools reservieren.",
        automaticUpdates: "Automatische Updates aktivieren",
        automaticUpdatesDescription: "Systemupdates automatisch herunterladen und installieren.",
        diagnosticData: "Diagnosedaten teilen",
        diagnosticDataDescription: "Helfen Sie, EagleOS durch das Teilen optionaler Diagnosedaten zu verbessern.",
        desktopShortcuts: "Desktopverknüpfungen erstellen",
        desktopShortcutsDescription: "Nützliche Verknüpfungen zum Desktop hinzufügen.",
        summaryTitle: "Bereit zur Installation von EagleOS",
        summaryDescription: "Überprüfen Sie Ihre Auswahl, bevor Sie fortfahren.",
        summaryLanguage: "Sprache",
        summaryRegion: "Region",
        summaryKeyboard: "Tastatur",
        summaryInstallation: "Installationstyp",
        summaryDestination: "Installationsort",
        summaryPreferences: "Einstellungen",
        clean: "Neuinstallation",
        upgrade: "Upgrade-Installation",
        summaryWarning: "Überprüfen Sie Ihre Auswahl. Änderungen während der Installation können möglicherweise nicht rückgängig gemacht werden.",
        installingTitle: "EagleOS wird installiert",
        installingDescription: "Bitte warten Sie, während EagleOS installiert wird.",
        installingDoNotTurnOff: "Schalten Sie Ihren Computer während der Installation nicht aus.",
        installingPreparing: "Installation wird vorbereitet...",
        installingFiles: "Systemdateien werden installiert...",
        installingConfiguration: "EagleOS wird konfiguriert...",
        installingFinishing: "Installation wird abgeschlossen...",
        installingProgress: "Installationsfortschritt",
        completeTitle: "EagleOS ist bereit!",
        completeDescription: "Die Installation wurde abgeschlossen.",
        completeMessage: "Vielen Dank, dass Sie EagleOS gewählt haben.",
        completeNote: "Ihr Computer ist bereit für den nächsten Schritt.",
        completeButton: "Fertigstellen",
        back: "Zurück",
        next: "Weiter",
        cancel: "Abbrechen",
        finish: "Fertigstellen",
        footer: "EagleOS-Installation",
        yes: "Ja",
        no: "Nein",
        enabled: "Aktiviert",
        disabled: "Deaktiviert"
    },

    Português: {
        welcome: "Bem-vindo",
        language: "Idioma",
        license: "Licença",
        integrity: "Integridade",
        region: "Região",
        keyboard: "Teclado",
        installation: "Instalação",
        destination: "Destino",
        preferences: "Preferências",
        summary: "Resumo",
        installing: "Instalando",
        complete: "Concluído",
        sidebar: "Instalação do EagleOS",
        sidebarStatus: "Instalação",
        welcomeTitle: "Bem-vindo ao EagleOS",
        welcomeDescription: "Vamos preparar seu computador para o EagleOS.",
        welcomeMessage: "Este assistente guiará você durante a instalação do EagleOS no seu computador.",
        welcomeNote: "Antes de começar, certifique-se de fazer backup dos seus arquivos importantes.",
        languageTitle: "Escolha seu idioma",
        languageDescription: "Selecione o idioma que deseja usar durante a instalação.",
        languageLabel: "Idioma de exibição",
        licenseTitle: "Contrato de licença do EagleOS",
        licenseDescription: "Leia o contrato de licença antes de continuar.",
        licenseAgreement: "Li e concordo com o contrato de licença.",
        integrityTitle: "Verificação de integridade do sistema",
        integrityDescription: "O EagleOS Setup verificará se os arquivos de instalação foram modificados.",
        integrityChecking: "Verificando arquivos de instalação...",
        integrityPassed: "Todos os arquivos passaram pela verificação de integridade.",
        integrityFailed: "Não foi possível verificar alguns arquivos de instalação.",
        integrityAgree: "Entendo os riscos e desejo continuar mesmo assim.",
        integrityAbort: "Não continuar com a instalação.",
        regionTitle: "Selecione sua região",
        regionDescription: "Escolha seu país ou região.",
        regionLabel: "País ou região",
        keyboardTitle: "Escolha o layout do teclado",
        keyboardDescription: "Selecione o layout do teclado conectado ao computador.",
        keyboardLabel: "Layout do teclado",
        installationTitle: "Escolha o tipo de instalação",
        installationDescription: "Selecione como deseja instalar o EagleOS.",
        cleanInstallation: "Instalação limpa",
        cleanDescription: "Instale uma nova cópia do EagleOS. Os dados existentes no destino selecionado podem ser apagados.",
        upgradeInstallation: "Atualização",
        upgradeDescription: "Atualize uma instalação existente do EagleOS mantendo arquivos e configurações compatíveis.",
        destinationTitle: "Escolha onde instalar o EagleOS",
        destinationDescription: "Selecione o dispositivo de armazenamento que deseja usar.",
        destinationLabel: "Dispositivos de armazenamento disponíveis",
        diskAvailable: "Disponível",
        diskCapacity: "Capacidade",
        diskType: "Tipo de unidade",
        destinationWarning: "Certifique-se de selecionar o dispositivo de armazenamento correto antes de continuar.",
        preferencesTitle: "Personalize sua instalação",
        preferencesDescription: "Escolha os recursos que deseja ativar no EagleOS.",
        recoveryPartition: "Criar uma partição de recuperação",
        recoveryPartitionDescription: "Reservar espaço para ferramentas de recuperação do sistema.",
        automaticUpdates: "Ativar atualizações automáticas",
        automaticUpdatesDescription: "Baixar e instalar atualizações do sistema automaticamente.",
        diagnosticData: "Compartilhar informações de diagnóstico",
        diagnosticDataDescription: "Ajude a melhorar o EagleOS compartilhando informações de diagnóstico opcionais.",
        desktopShortcuts: "Criar atalhos na área de trabalho",
        desktopShortcutsDescription: "Adicionar atalhos úteis à sua área de trabalho.",
        summaryTitle: "Pronto para instalar o EagleOS",
        summaryDescription: "Revise suas escolhas antes de continuar.",
        summaryLanguage: "Idioma",
        summaryRegion: "Região",
        summaryKeyboard: "Teclado",
        summaryInstallation: "Tipo de instalação",
        summaryDestination: "Destino",
        summaryPreferences: "Preferências",
        clean: "Instalação limpa",
        upgrade: "Atualização",
        summaryWarning: "Revise suas escolhas. Talvez não seja possível desfazer as alterações feitas durante a instalação.",
        installingTitle: "Instalando o EagleOS",
        installingDescription: "Aguarde enquanto o EagleOS é instalado.",
        installingDoNotTurnOff: "Não desligue o computador durante a instalação.",
        installingPreparing: "Preparando a instalação...",
        installingFiles: "Instalando arquivos do sistema...",
        installingConfiguration: "Configurando o EagleOS...",
        installingFinishing: "Finalizando a instalação...",
        installingProgress: "Progresso da instalação",
        completeTitle: "O EagleOS está pronto!",
        completeDescription: "O processo de instalação foi concluído.",
        completeMessage: "Obrigado por escolher o EagleOS.",
        completeNote: "Seu computador está pronto para a próxima etapa.",
        completeButton: "Concluir",
        back: "Voltar",
        next: "Avançar",
        cancel: "Cancelar",
        finish: "Concluir",
        footer: "Instalação do EagleOS",
        yes: "Sim",
        no: "Não",
        enabled: "Ativado",
        disabled: "Desativado"
    },

    Italiano: {
        welcome: "Benvenuto",
        language: "Lingua",
        license: "Licenza",
        integrity: "Integrità",
        region: "Regione",
        keyboard: "Tastiera",
        installation: "Installazione",
        destination: "Destinazione",
        preferences: "Preferenze",
        summary: "Riepilogo",
        installing: "Installazione in corso",
        complete: "Completato",
        sidebar: "Installazione di EagleOS",
        sidebarStatus: "Installazione",
        welcomeTitle: "Benvenuto in EagleOS",
        welcomeDescription: "Prepariamo il tuo computer per EagleOS.",
        welcomeMessage: "Questa procedura ti guiderà nell'installazione di EagleOS sul tuo computer.",
        welcomeNote: "Prima di iniziare, assicurati di aver eseguito il backup dei file importanti.",
        languageTitle: "Scegli la lingua",
        languageDescription: "Seleziona la lingua da utilizzare durante l'installazione.",
        languageLabel: "Lingua di visualizzazione",
        licenseTitle: "Contratto di licenza EagleOS",
        licenseDescription: "Leggi il contratto di licenza prima di continuare.",
        licenseAgreement: "Ho letto e accetto il contratto di licenza.",
        integrityTitle: "Verifica dell'integrità del sistema",
        integrityDescription: "EagleOS Setup verificherà che i file di installazione non siano stati modificati.",
        integrityChecking: "Verifica dei file di installazione...",
        integrityPassed: "Tutti i file hanno superato la verifica di integrità.",
        integrityFailed: "Impossibile verificare alcuni file di installazione.",
        integrityAgree: "Comprendo i rischi e desidero continuare comunque.",
        integrityAbort: "Non continuare l'installazione.",
        regionTitle: "Seleziona la tua regione",
        regionDescription: "Scegli il tuo paese o la tua regione.",
        regionLabel: "Paese o regione",
        keyboardTitle: "Scegli il layout della tastiera",
        keyboardDescription: "Seleziona il layout della tastiera collegata al computer.",
        keyboardLabel: "Layout della tastiera",
        installationTitle: "Scegli il tipo di installazione",
        installationDescription: "Seleziona come desideri installare EagleOS.",
        cleanInstallation: "Installazione pulita",
        cleanDescription: "Installa una nuova copia di EagleOS. I dati esistenti nella destinazione selezionata potrebbero essere eliminati.",
        upgradeInstallation: "Aggiornamento",
        upgradeDescription: "Aggiorna un'installazione esistente di EagleOS mantenendo i file e le impostazioni compatibili.",
        destinationTitle: "Scegli dove installare EagleOS",
        destinationDescription: "Seleziona il dispositivo di archiviazione da utilizzare.",
        destinationLabel: "Dispositivi di archiviazione disponibili",
        diskAvailable: "Disponibile",
        diskCapacity: "Capacità",
        diskType: "Tipo di unità",
        destinationWarning: "Assicurati di selezionare il dispositivo di archiviazione corretto prima di continuare.",
        preferencesTitle: "Personalizza l'installazione",
        preferencesDescription: "Scegli le funzionalità da attivare in EagleOS.",
        recoveryPartition: "Crea una partizione di ripristino",
        recoveryPartitionDescription: "Riserva spazio per gli strumenti di ripristino del sistema.",
        automaticUpdates: "Attiva gli aggiornamenti automatici",
        automaticUpdatesDescription: "Scarica e installa automaticamente gli aggiornamenti del sistema.",
        diagnosticData: "Condividi informazioni diagnostiche",
        diagnosticDataDescription: "Aiuta a migliorare EagleOS condividendo informazioni diagnostiche facoltative.",
        desktopShortcuts: "Crea collegamenti sul desktop",
        desktopShortcutsDescription: "Aggiungi collegamenti utili al desktop.",
        summaryTitle: "Pronto per installare EagleOS",
        summaryDescription: "Controlla le tue scelte prima di continuare.",
        summaryLanguage: "Lingua",
        summaryRegion: "Regione",
        summaryKeyboard: "Tastiera",
        summaryInstallation: "Tipo di installazione",
        summaryDestination: "Destinazione",
        summaryPreferences: "Preferenze",
        clean: "Installazione pulita",
        upgrade: "Aggiornamento",
        summaryWarning: "Controlla le tue scelte. Potrebbe non essere possibile annullare le modifiche apportate durante l'installazione.",
        installingTitle: "Installazione di EagleOS",
        installingDescription: "Attendi mentre EagleOS viene installato.",
        installingDoNotTurnOff: "Non spegnere il computer durante l'installazione.",
        installingPreparing: "Preparazione dell'installazione...",
        installingFiles: "Installazione dei file di sistema...",
        installingConfiguration: "Configurazione di EagleOS...",
        installingFinishing: "Completamento dell'installazione...",
        installingProgress: "Avanzamento dell'installazione",
        completeTitle: "EagleOS è pronto!",
        completeDescription: "Il processo di installazione è terminato.",
        completeMessage: "Grazie per aver scelto EagleOS.",
        completeNote: "Il computer è pronto per il passaggio successivo.",
        completeButton: "Fine",
        back: "Indietro",
        next: "Avanti",
        cancel: "Annulla",
        finish: "Fine",
        footer: "Installazione di EagleOS",
        yes: "Sì",
        no: "No",
        enabled: "Attivato",
        disabled: "Disattivato"
    },

    日本語: {
        welcome: "ようこそ",
        language: "言語",
        license: "ライセンス",
        integrity: "整合性",
        region: "地域",
        keyboard: "キーボード",
        installation: "インストール",
        destination: "インストール先",
        preferences: "設定",
        summary: "概要",
        installing: "インストール中",
        complete: "完了",
        sidebar: "EagleOS セットアップ",
        sidebarStatus: "インストール",
        welcomeTitle: "EagleOS へようこそ",
        welcomeDescription: "EagleOS を使用するためにコンピューターを準備しましょう。",
        welcomeMessage: "このセットアップでは、コンピューターへの EagleOS のインストールを案内します。",
        welcomeNote: "開始する前に、重要なファイルをバックアップしてください。",
        languageTitle: "言語を選択",
        languageDescription: "セットアップ中に使用する言語を選択してください。",
        languageLabel: "表示言語",
        licenseTitle: "EagleOS 使用許諾契約",
        licenseDescription: "続行する前に使用許諾契約を確認してください。",
        licenseAgreement: "使用許諾契約を読み、同意します。",
        integrityTitle: "システム整合性チェック",
        integrityDescription: "EagleOS Setup はインストールファイルが変更されていないことを確認します。",
        integrityChecking: "インストールファイルを確認しています...",
        integrityPassed: "すべてのインストールファイルの検証に成功しました。",
        integrityFailed: "一部のインストールファイルを検証できませんでした。",
        integrityAgree: "リスクを理解したうえで続行します。",
        integrityAbort: "インストールを続行しない。",
        regionTitle: "地域を選択",
        regionDescription: "国または地域を選択してください。",
        regionLabel: "国または地域",
        keyboardTitle: "キーボードレイアウトを選択",
        keyboardDescription: "コンピューターに接続されているキーボードのレイアウトを選択してください。",
        keyboardLabel: "キーボードレイアウト",
        installationTitle: "インストールの種類を選択",
        installationDescription: "EagleOS のインストール方法を選択してください。",
        cleanInstallation: "新規インストール",
        cleanDescription: "EagleOS を新しくインストールします。選択した保存先の既存データは削除される場合があります。",
        upgradeInstallation: "アップグレード",
        upgradeDescription: "互換性のあるファイルと設定を保持しながら、既存の EagleOS を更新します。",
        destinationTitle: "EagleOS のインストール先を選択",
        destinationDescription: "使用するストレージデバイスを選択してください。",
        destinationLabel: "使用可能なストレージデバイス",
        diskAvailable: "空き容量",
        diskCapacity: "容量",
        diskType: "ドライブの種類",
        destinationWarning: "続行する前に、正しいストレージデバイスを選択したことを確認してください。",
        preferencesTitle: "インストールをカスタマイズ",
        preferencesDescription: "EagleOS で有効にする機能を選択してください。",
        recoveryPartition: "回復パーティションを作成",
        recoveryPartitionDescription: "システム回復ツール用の領域を確保します。",
        automaticUpdates: "自動更新を有効にする",
        automaticUpdatesDescription: "システム更新を自動的にダウンロードしてインストールします。",
        diagnosticData: "診断情報を共有",
        diagnosticDataDescription: "任意の診断情報を共有して EagleOS の改善に協力します。",
        desktopShortcuts: "デスクトップショートカットを作成",
        desktopShortcutsDescription: "便利なショートカットをデスクトップに追加します。",
        summaryTitle: "EagleOS をインストールする準備ができました",
        summaryDescription: "続行する前にインストール設定を確認してください。",
        summaryLanguage: "言語",
        summaryRegion: "地域",
        summaryKeyboard: "キーボード",
        summaryInstallation: "インストールの種類",
        summaryDestination: "インストール先",
        summaryPreferences: "設定",
        clean: "新規インストール",
        upgrade: "アップグレード",
        summaryWarning: "設定を確認してください。インストール中に行われた変更は元に戻せない場合があります。",
        installingTitle: "EagleOS をインストールしています",
        installingDescription: "EagleOS のインストールが完了するまでお待ちください。",
        installingDoNotTurnOff: "インストール中はコンピューターの電源を切らないでください。",
        installingPreparing: "インストールを準備しています...",
        installingFiles: "システムファイルをインストールしています...",
        installingConfiguration: "EagleOS を設定しています...",
        installingFinishing: "インストールを完了しています...",
        installingProgress: "インストールの進行状況",
        completeTitle: "EagleOS の準備ができました！",
        completeDescription: "インストールが完了しました。",
        completeMessage: "EagleOS をお選びいただきありがとうございます。",
        completeNote: "コンピューターは次のステップに進む準備ができています。",
        completeButton: "完了",
        back: "戻る",
        next: "次へ",
        cancel: "キャンセル",
        finish: "完了",
        footer: "EagleOS セットアップ",
        yes: "はい",
        no: "いいえ",
        enabled: "有効",
        disabled: "無効"
    },

    한국어: {
        welcome: "환영합니다",
        language: "언어",
        license: "라이선스",
        integrity: "무결성",
        region: "지역",
        keyboard: "키보드",
        installation: "설치",
        destination: "설치 위치",
        preferences: "환경 설정",
        summary: "요약",
        installing: "설치 중",
        complete: "완료",
        sidebar: "EagleOS 설치",
        sidebarStatus: "설치",
        welcomeTitle: "EagleOS에 오신 것을 환영합니다",
        welcomeDescription: "EagleOS를 사용할 수 있도록 컴퓨터를 준비하겠습니다.",
        welcomeMessage: "이 설치 프로그램은 컴퓨터에 EagleOS를 설치하는 과정을 안내합니다.",
        welcomeNote: "시작하기 전에 중요한 파일을 백업했는지 확인하세요.",
        languageTitle: "언어 선택",
        languageDescription: "설치 중에 사용할 언어를 선택하세요.",
        languageLabel: "표시 언어",
        licenseTitle: "EagleOS 사용권 계약",
        licenseDescription: "계속하기 전에 사용권 계약을 확인하세요.",
        licenseAgreement: "사용권 계약을 읽었으며 동의합니다.",
        integrityTitle: "시스템 무결성 검사",
        integrityDescription: "EagleOS 설치 프로그램이 설치 파일의 변경 여부를 확인합니다.",
        integrityChecking: "설치 파일을 확인하는 중...",
        integrityPassed: "모든 설치 파일의 무결성 검사가 완료되었습니다.",
        integrityFailed: "일부 설치 파일을 확인할 수 없습니다.",
        integrityAgree: "위험을 이해했으며 계속 진행하겠습니다.",
        integrityAbort: "설치를 계속하지 않습니다.",
        regionTitle: "지역 선택",
        regionDescription: "국가 또는 지역을 선택하세요.",
        regionLabel: "국가 또는 지역",
        keyboardTitle: "키보드 레이아웃 선택",
        keyboardDescription: "컴퓨터에 연결된 키보드의 레이아웃을 선택하세요.",
        keyboardLabel: "키보드 레이아웃",
        installationTitle: "설치 유형 선택",
        installationDescription: "EagleOS 설치 방법을 선택하세요.",
        cleanInstallation: "새로 설치",
        cleanDescription: "EagleOS를 새로 설치합니다. 선택한 위치의 기존 데이터가 삭제될 수 있습니다.",
        upgradeInstallation: "업그레이드 설치",
        upgradeDescription: "호환되는 파일과 설정을 유지하면서 기존 EagleOS를 업그레이드합니다.",
        destinationTitle: "EagleOS 설치 위치 선택",
        destinationDescription: "사용할 저장 장치를 선택하세요.",
        destinationLabel: "사용 가능한 저장 장치",
        diskAvailable: "사용 가능",
        diskCapacity: "용량",
        diskType: "드라이브 유형",
        destinationWarning: "계속하기 전에 올바른 저장 장치를 선택했는지 확인하세요.",
        preferencesTitle: "설치 사용자 지정",
        preferencesDescription: "EagleOS에서 사용할 기능을 선택하세요.",
        recoveryPartition: "복구 파티션 만들기",
        recoveryPartitionDescription: "시스템 복구 도구를 위한 공간을 확보합니다.",
        automaticUpdates: "자동 업데이트 사용",
        automaticUpdatesDescription: "시스템 업데이트를 자동으로 다운로드하고 설치합니다.",
        diagnosticData: "진단 정보 공유",
        diagnosticDataDescription: "선택적 진단 정보를 공유하여 EagleOS 개선에 도움을 줍니다.",
        desktopShortcuts: "바탕 화면 바로 가기 만들기",
        desktopShortcutsDescription: "유용한 바로 가기를 바탕 화면에 추가합니다.",
        summaryTitle: "EagleOS 설치 준비 완료",
        summaryDescription: "계속하기 전에 설치 선택 사항을 확인하세요.",
        summaryLanguage: "언어",
        summaryRegion: "지역",
        summaryKeyboard: "키보드",
        summaryInstallation: "설치 유형",
        summaryDestination: "설치 위치",
        summaryPreferences: "환경 설정",
        clean: "새로 설치",
        upgrade: "업그레이드 설치",
        summaryWarning: "선택 사항을 확인하세요. 설치 중 변경한 내용은 되돌릴 수 없을 수 있습니다.",
        installingTitle: "EagleOS 설치 중",
        installingDescription: "EagleOS가 설치되는 동안 기다려 주세요.",
        installingDoNotTurnOff: "설치 중에는 컴퓨터 전원을 끄지 마세요.",
        installingPreparing: "설치 준비 중...",
        installingFiles: "시스템 파일 설치 중...",
        installingConfiguration: "EagleOS 구성 중...",
        installingFinishing: "설치 마무리 중...",
        installingProgress: "설치 진행률",
        completeTitle: "EagleOS 설치가 완료되었습니다!",
        completeDescription: "설치 과정이 완료되었습니다.",
        completeMessage: "EagleOS를 선택해 주셔서 감사합니다.",
        completeNote: "컴퓨터가 다음 단계를 진행할 준비가 되었습니다.",
        completeButton: "마침",
        back: "뒤로",
        next: "다음",
        cancel: "취소",
        finish: "마침",
        footer: "EagleOS 설치",
        yes: "예",
        no: "아니요",
        enabled: "사용",
        disabled: "사용 안 함"
    },

    中文: {
        welcome: "欢迎",
        language: "语言",
        license: "许可协议",
        integrity: "完整性",
        region: "地区",
        keyboard: "键盘",
        installation: "安装",
        destination: "安装位置",
        preferences: "首选项",
        summary: "摘要",
        installing: "正在安装",
        complete: "完成",
        sidebar: "EagleOS 安装程序",
        sidebarStatus: "安装",
        welcomeTitle: "欢迎使用 EagleOS",
        welcomeDescription: "让我们为使用 EagleOS 做好电脑准备。",
        welcomeMessage: "本安装程序将指导您在电脑上安装 EagleOS。",
        welcomeNote: "开始之前，请确保已备份所有重要文件。",
        languageTitle: "选择语言",
        languageDescription: "选择安装过程中使用的语言。",
        languageLabel: "显示语言",
        licenseTitle: "EagleOS 许可协议",
        licenseDescription: "继续之前，请阅读许可协议。",
        licenseAgreement: "我已阅读并同意许可协议。",
        integrityTitle: "系统完整性检查",
        integrityDescription: "EagleOS 安装程序将验证安装文件是否被修改。",
        integrityChecking: "正在检查安装文件...",
        integrityPassed: "所有安装文件均已通过完整性验证。",
        integrityFailed: "部分安装文件无法通过验证。",
        integrityAgree: "我了解相关风险，仍要继续。",
        integrityAbort: "不要继续安装。",
        regionTitle: "选择地区",
        regionDescription: "选择您的国家或地区。",
        regionLabel: "国家或地区",
        keyboardTitle: "选择键盘布局",
        keyboardDescription: "选择连接到电脑的键盘布局。",
        keyboardLabel: "键盘布局",
        installationTitle: "选择安装类型",
        installationDescription: "选择 EagleOS 的安装方式。",
        cleanInstallation: "全新安装",
        cleanDescription: "安装全新的 EagleOS。所选目标位置中的现有数据可能会被删除。",
        upgradeInstallation: "升级安装",
        upgradeDescription: "升级现有的 EagleOS，同时保留兼容的文件和设置。",
        destinationTitle: "选择 EagleOS 的安装位置",
        destinationDescription: "选择要使用的存储设备。",
        destinationLabel: "可用存储设备",
        diskAvailable: "可用空间",
        diskCapacity: "容量",
        diskType: "驱动器类型",
        destinationWarning: "继续之前，请确认已选择正确的存储设备。",
        preferencesTitle: "自定义安装",
        preferencesDescription: "选择要在 EagleOS 中启用的功能。",
        recoveryPartition: "创建恢复分区",
        recoveryPartitionDescription: "为系统恢复工具预留空间。",
        automaticUpdates: "启用自动更新",
        automaticUpdatesDescription: "自动下载并安装系统更新。",
        diagnosticData: "共享诊断信息",
        diagnosticDataDescription: "通过共享可选诊断信息帮助改进 EagleOS。",
        desktopShortcuts: "创建桌面快捷方式",
        desktopShortcutsDescription: "向桌面添加实用的快捷方式。",
        summaryTitle: "已准备好安装 EagleOS",
        summaryDescription: "继续之前，请检查您的安装选项。",
        summaryLanguage: "语言",
        summaryRegion: "地区",
        summaryKeyboard: "键盘",
        summaryInstallation: "安装类型",
        summaryDestination: "安装位置",
        summaryPreferences: "首选项",
        clean: "全新安装",
        upgrade: "升级安装",
        summaryWarning: "请检查您的选择。安装过程中所做的更改可能无法撤销。",
        installingTitle: "正在安装 EagleOS",
        installingDescription: "请等待 EagleOS 安装完成。",
        installingDoNotTurnOff: "安装过程中请勿关闭电脑。",
        installingPreparing: "正在准备安装...",
        installingFiles: "正在安装系统文件...",
        installingConfiguration: "正在配置 EagleOS...",
        installingFinishing: "正在完成安装...",
        installingProgress: "安装进度",
        completeTitle: "EagleOS 已准备就绪！",
        completeDescription: "安装过程已完成。",
        completeMessage: "感谢您选择 EagleOS。",
        completeNote: "您的电脑已准备好进入下一步。",
        completeButton: "完成",
        back: "上一步",
        next: "下一步",
        cancel: "取消",
        finish: "完成",
        footer: "EagleOS 安装程序",
        yes: "是",
        no: "否",
        enabled: "已启用",
        disabled: "已禁用"
    }
};

let settings = loadSettings();

let currentStep = 0;
let currentScreen = "welcome";

let installationRunning = false;
let installationTimer = null;
let installationProgress = 0;

let integrityResult = null;
let integrityCheckPromise = null;
let integrityAcknowledged = false;
let integrityChecking = false;

let screenContent;
let backButton;
let nextButton;
let footerStatus;

function cloneDefaults() {
    return JSON.parse(JSON.stringify(DEFAULT_SETTINGS));
}

function loadSettings() {
    const defaults = cloneDefaults();

    try {
        const saved = JSON.parse(localStorage.getItem(STORAGE_KEY));

        if (!saved || typeof saved !== "object") {
            return defaults;
        }

        return {
            ...defaults,
            ...saved,

            language: LANGUAGE_NAMES.includes(saved.language)
                ? saved.language
                : defaults.language,

            region: REGIONS.includes(saved.region)
                ? saved.region
                : defaults.region,

            keyboard: KEYBOARD_LAYOUTS.includes(saved.keyboard)
                ? saved.keyboard
                : defaults.keyboard,

            installationType: ["clean", "upgrade"].includes(saved.installationType)
                ? saved.installationType
                : defaults.installationType,

            destination: DISKS.some(disk => disk.id === saved.destination)
                ? saved.destination
                : defaults.destination,

            preferences: {
                ...defaults.preferences,
                ...(saved.preferences || {})
            },

            preferencesCompleted: Boolean(saved.preferencesCompleted)
        };

    } catch (error) {
        console.error("Unable to load installer settings:", error);
        return defaults;
    }
}

function saveSettings() {
    try {
        localStorage.setItem(STORAGE_KEY, JSON.stringify(settings));
    } catch (error) {
        console.error("Unable to save installer settings:", error);
    }
}

function t(key) {
    const language = TRANSLATIONS[settings.language] || TRANSLATIONS.English;

    return language[key] ?? TRANSLATIONS.English[key] ?? key;
}

function escapeHTML(value) {
    return String(value)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;")
        .replace(/'/g, "&#039;");
}

function createScreen(icon, title, description, content) {
    return `
        <div class="screen">
            <div class="screen-header">
                <div class="screen-icon">${icon}</div>

                <div>
                    <h1>${title}</h1>
                    <p>${description}</p>
                </div>
            </div>

            <div class="screen-body">
                ${content}
            </div>
        </div>
    `;
}

function getSelectedDisk() {
    return DISKS.find(disk => disk.id === settings.destination) || DISKS[0];
}

function renderSidebar() {
    const sidebarHeading = document.getElementById("sidebar-heading");
    const sidebarStatus = document.getElementById("sidebar-status");
    const stepsContainer = document.getElementById("installer-steps");

    if (sidebarHeading) {
        sidebarHeading.textContent = t("sidebar");
    }

    if (sidebarStatus) {
        sidebarStatus.textContent = t("sidebarStatus");
    }

    if (!stepsContainer) {
        console.error("Installer sidebar navigation was not found.");
        return;
    }

    stepsContainer.innerHTML = "";

    SCREENS.forEach((screen, index) => {
        const step = document.createElement("div");

        step.className = "step-item";

        if (index === currentStep) {
            step.classList.add("active");
        }

        if (index < currentStep) {
            step.classList.add("completed");
        }

        step.innerHTML = `
            <div class="step-number">${index + 1}</div>
            <div class="step-label">${escapeHTML(t(screen))}</div>
        `;

        stepsContainer.appendChild(step);
    });
}

function renderWelcome() {
    const content = `
        <div class="welcome-content">
            <div class="welcome-logo">🦅</div>

            <h2>${t("welcomeMessage")}</h2>

            <p>${t("welcomeNote")}</p>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "👋",
        t("welcomeTitle"),
        t("welcomeDescription"),
        content
    );
}

function renderLanguage() {
    const options = LANGUAGE_NAMES.map(language => `
        <option value="${escapeHTML(language)}"
            ${settings.language === language ? "selected" : ""}>
            ${escapeHTML(language)}
        </option>
    `).join("");

    const content = `
        <div class="form-group">
            <label for="language-select">${t("languageLabel")}</label>

            <select id="language-select" class="installer-select">
                ${options}
            </select>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "🌐",
        t("languageTitle"),
        t("languageDescription"),
        content
    );

    const select = document.getElementById("language-select");

    select.addEventListener("change", event => {
        settings.language = event.target.value;

        saveSettings();
        renderStep();
    });
}

function renderLicense() {
    if (!window.EagleOSLicense) {
        screenContent.innerHTML = createScreen(
            "📄",
            t("licenseTitle"),
            t("licenseDescription"),
            `<p>License component could not be loaded.</p>`
        );

        nextButton.disabled = true;
        return;
    }

    window.EagleOSLicense.reset();

    const accepted = false;
    
    const licenseText = escapeHTML(window.EagleOSLicense.text);

    const content = `
        <div class="license-container">
            <div class="license-document">
                <pre>${licenseText}</pre>
            </div>

            <label class="installer-checkbox">
                <input
                    type="checkbox"
                    id="license-agreement"
                    ${accepted ? "checked" : ""}
                >

                <span>${t("licenseAgreement")}</span>
            </label>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "📄",
        t("licenseTitle"),
        t("licenseDescription"),
        content
    );

    const checkbox = document.getElementById("license-agreement");

    nextButton.disabled = !checkbox.checked;

    checkbox.addEventListener("change", () => {
        if (checkbox.checked) {
            window.EagleOSLicense.accept();
        } else {
            window.EagleOSLicense.reset();
        }

        nextButton.disabled = !checkbox.checked;
    });
}

async function renderIntegrity() {
    const content = `
        <div class="integrity-container">
            <div id="integrity-result" class="integrity-result">
                <div class="integrity-spinner"></div>
                <p>${t("integrityChecking")}</p>
            </div>

            <div id="integrity-issues"></div>

            <label
                class="installer-checkbox"
                id="integrity-acknowledgement-container"
                hidden
            >
                <input
                    type="checkbox"
                    id="integrity-acknowledgement"
                >

                <span>${t("integrityAgree")}</span>
            </label>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "🛡️",
        t("integrityTitle"),
        t("integrityDescription"),
        content
    );

    const resultContainer = document.getElementById("integrity-result");
    const issuesContainer = document.getElementById("integrity-issues");
    const acknowledgementContainer = document.getElementById(
        "integrity-acknowledgement-container"
    );

    integrityAcknowledged = false;
    integrityChecking = true;

    nextButton.disabled = true;

    if (!window.EagleOSIntegrity) {
        integrityResult = {
            verified: false,
            issues: [
                {
                    path: "integrity.js",
                    reason: "The integrity verification component could not be loaded."
                }
            ]
        };
    } else {
        if (!integrityCheckPromise) {
            integrityCheckPromise = window.EagleOSIntegrity.verify();
        }

        try {
            integrityResult = await integrityCheckPromise;
        } catch (error) {
            integrityResult = {
                verified: false,
                issues: [
                    {
                        path: "Integrity verification",
                        reason: error.message || "An unexpected error occurred."
                    }
                ]
            };
        }
    }

    integrityChecking = false;

    if (currentScreen !== "integrity") {
        return;
    }

    if (integrityResult.verified) {
        resultContainer.innerHTML = `
            <div class="integrity-success">
                <span>✓</span>
                <h3>${t("integrityPassed")}</h3>
            </div>
        `;

        nextButton.disabled = false;

    } else {
        resultContainer.innerHTML = `
            <div class="integrity-failure">
                <span>⚠</span>
                <h3>${t("integrityFailed")}</h3>
            </div>
        `;

        issuesContainer.innerHTML = `
            <div class="integrity-issue-list">
                ${integrityResult.issues.map(issue => `
                    <div class="integrity-issue">
                        <strong>${escapeHTML(issue.path)}</strong>
                        <p>${escapeHTML(issue.reason)}</p>
                    </div>
                `).join("")}
            </div>
        `;

        acknowledgementContainer.hidden = false;

        const checkbox = document.getElementById(
            "integrity-acknowledgement"
        );

        checkbox.addEventListener("change", () => {
            integrityAcknowledged = checkbox.checked;
            nextButton.disabled = !integrityAcknowledged;
        });

        nextButton.disabled = true;
    }
}

function renderRegion() {
    const options = REGIONS.map(region => `
        <option value="${escapeHTML(region)}"
            ${settings.region === region ? "selected" : ""}>
            ${escapeHTML(region)}
        </option>
    `).join("");

    const content = `
        <div class="form-group">
            <label for="region-select">${t("regionLabel")}</label>

            <select id="region-select" class="installer-select">
                ${options}
            </select>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "📍",
        t("regionTitle"),
        t("regionDescription"),
        content
    );

    document.getElementById("region-select").addEventListener("change", event => {
        settings.region = event.target.value;
        saveSettings();
    });
}

function renderKeyboard() {
    const options = KEYBOARD_LAYOUTS.map(layout => `
        <option value="${escapeHTML(layout)}"
            ${settings.keyboard === layout ? "selected" : ""}>
            ${escapeHTML(layout)}
        </option>
    `).join("");

    const content = `
        <div class="form-group">
            <label for="keyboard-select">${t("keyboardLabel")}</label>

            <select id="keyboard-select" class="installer-select">
                ${options}
            </select>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "⌨️",
        t("keyboardTitle"),
        t("keyboardDescription"),
        content
    );

    document.getElementById("keyboard-select").addEventListener("change", event => {
        settings.keyboard = event.target.value;
        saveSettings();
    });
}

function renderInstallation() {
    const content = `
        <div class="installation-options">
            <label class="installation-option">
                <input
                    type="radio"
                    name="installation-type"
                    value="clean"
                    ${settings.installationType === "clean" ? "checked" : ""}
                >

                <div>
                    <h3>${t("cleanInstallation")}</h3>
                    <p>${t("cleanDescription")}</p>
                </div>
            </label>

            <label class="installation-option">
                <input
                    type="radio"
                    name="installation-type"
                    value="upgrade"
                    ${settings.installationType === "upgrade" ? "checked" : ""}
                >

                <div>
                    <h3>${t("upgradeInstallation")}</h3>
                    <p>${t("upgradeDescription")}</p>
                </div>
            </label>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "💿",
        t("installationTitle"),
        t("installationDescription"),
        content
    );

    document.querySelectorAll('input[name="installation-type"]').forEach(input => {
        input.addEventListener("change", event => {
            settings.installationType = event.target.value;
            saveSettings();
        });
    });
}

function renderDestination() {
    const disks = DISKS.map(disk => `
        <label class="disk-option">
            <input
                type="radio"
                name="destination"
                value="${disk.id}"
                ${settings.destination === disk.id ? "checked" : ""}
            >

            <div class="disk-information">
                <div class="disk-heading">
                    <strong>${escapeHTML(disk.name)}</strong>
                    <span>${escapeHTML(disk.type)}</span>
                </div>

                <p>${escapeHTML(disk.model)}</p>

                <div class="disk-details">
                    <span>${t("diskCapacity")}: ${escapeHTML(disk.capacity)}</span>
                    <span>${t("diskAvailable")}: ${escapeHTML(disk.available)}</span>
                </div>
            </div>
        </label>
    `).join("");

    const content = `
        <div class="disk-list">
            ${disks}
        </div>

        <div class="destination-warning">
            ⚠️ ${t("destinationWarning")}
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "💾",
        t("destinationTitle"),
        t("destinationDescription"),
        content
    );

    document.querySelectorAll('input[name="destination"]').forEach(input => {
        input.addEventListener("change", event => {
            settings.destination = event.target.value;
            saveSettings();
        });
    });
}

function renderPreferences() {
    const preferences = [
        {
            id: "recoveryPartition",
            title: t("recoveryPartition"),
            description: t("recoveryPartitionDescription")
        },
        {
            id: "automaticUpdates",
            title: t("automaticUpdates"),
            description: t("automaticUpdatesDescription")
        },
        {
            id: "diagnosticData",
            title: t("diagnosticData"),
            description: t("diagnosticDataDescription")
        },
        {
            id: "desktopShortcuts",
            title: t("desktopShortcuts"),
            description: t("desktopShortcutsDescription")
        }
    ];

    const options = preferences.map(preference => `
        <label class="preference-option">
            <input
                type="checkbox"
                data-preference="${preference.id}"
                ${settings.preferences[preference.id] ? "checked" : ""}
            >

            <div>
                <strong>${preference.title}</strong>
                <p>${preference.description}</p>
            </div>
        </label>
    `).join("");

    screenContent.innerHTML = createScreen(
        "⚙️",
        t("preferencesTitle"),
        t("preferencesDescription"),
        `<div class="preferences-list">${options}</div>`
    );

    document.querySelectorAll("[data-preference]").forEach(input => {
        input.addEventListener("change", event => {
            const preference = event.target.dataset.preference;

            settings.preferences[preference] = event.target.checked;
            settings.preferencesCompleted = true;

            saveSettings();
        });
    });
}

function renderSummary() {
    const disk = getSelectedDisk();

    const preferenceRows = Object.entries(settings.preferences).map(([key, enabled]) => {
        const labelKey = key;

        return `
            <div class="summary-row">
                <span>${t(labelKey)}</span>
                <strong>${enabled ? t("enabled") : t("disabled")}</strong>
            </div>
        `;
    }).join("");

    const content = `
        <div class="summary-card">
            <div class="summary-section">
                <h3>${t("summaryTitle")}</h3>

                <div class="summary-row">
                    <span>${t("summaryLanguage")}</span>
                    <strong>${escapeHTML(settings.language)}</strong>
                </div>

                <div class="summary-row">
                    <span>${t("summaryRegion")}</span>
                    <strong>${escapeHTML(settings.region)}</strong>
                </div>

                <div class="summary-row">
                    <span>${t("summaryKeyboard")}</span>
                    <strong>${escapeHTML(settings.keyboard)}</strong>
                </div>

                <div class="summary-row">
                    <span>${t("summaryInstallation")}</span>
                    <strong>${t(settings.installationType)}</strong>
                </div>

                <div class="summary-row">
                    <span>${t("summaryDestination")}</span>
                    <strong>${escapeHTML(disk.name)} — ${escapeHTML(disk.capacity)}</strong>
                </div>
            </div>

            <div class="summary-section">
                <h3>${t("summaryPreferences")}</h3>
                ${preferenceRows}
            </div>
        </div>

        <div class="summary-warning">
            ⚠️ ${t("summaryWarning")}
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "📋",
        t("summaryTitle"),
        t("summaryDescription"),
        content
    );
}

function renderInstalling() {
    const content = `
        <div class="installing-container">
            <div class="installing-icon">🦅</div>

            <h2 id="installation-status">
                ${t("installingPreparing")}
            </h2>

            <div class="installation-progress-container">
                <div class="installation-progress-track">
                    <div
                        id="installation-progress"
                        class="installation-progress"
                        style="width: 0%"
                    ></div>
                </div>

                <div class="installation-progress-details">
                    <span>${t("installingProgress")}</span>
                    <strong id="installation-percentage">0%</strong>
                </div>
            </div>

            <p class="installation-warning">
                ${t("installingDoNotTurnOff")}
            </p>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "⚙️",
        t("installingTitle"),
        t("installingDescription"),
        content
    );

    nextButton.hidden = true;
    backButton.hidden = true;

    startInstallation();
}

function startInstallation() {
    if (installationRunning) {
        return;
    }

    installationRunning = true;
    installationProgress = 0;

    const stages = [
        {
            progress: 15,
            message: "installingPreparing"
        },
        {
            progress: 70,
            message: "installingFiles"
        },
        {
            progress: 90,
            message: "installingConfiguration"
        },
        {
            progress: 100,
            message: "installingFinishing"
        }
    ];

    installationTimer = setInterval(() => {
        if (currentScreen !== "installing") {
            clearInterval(installationTimer);
            installationTimer = null;
            installationRunning = false;
            return;
        }

        installationProgress = Math.min(
            installationProgress + 1,
            100
        );

        const progressBar = document.getElementById("installation-progress");
        const percentage = document.getElementById("installation-percentage");
        const status = document.getElementById("installation-status");

        if (progressBar) {
            progressBar.style.width = `${installationProgress}%`;
        }

        if (percentage) {
            percentage.textContent = `${installationProgress}%`;
        }

        const stage = stages.find(item => installationProgress <= item.progress);

        if (status && stage) {
            status.textContent = t(stage.message);
        }

        if (installationProgress >= 100) {
            clearInterval(installationTimer);
            installationTimer = null;
            installationRunning = false;

            saveSettings();

            setTimeout(() => {
                currentStep = SCREENS.indexOf("complete");
                renderStep();
            }, 800);
        }
    }, 100);
}

function renderComplete() {
    const content = `
        <div class="complete-container">
            <div class="complete-icon">✓</div>

            <h2>${t("completeMessage")}</h2>

            <p>${t("completeNote")}</p>
        </div>
    `;

    screenContent.innerHTML = createScreen(
        "🎉",
        t("completeTitle"),
        t("completeDescription"),
        content
    );

    nextButton.textContent = t("finish");
    nextButton.hidden = false;
    backButton.hidden = true;
    nextButton.disabled = false;
}

function renderStep() {
    if (!screenContent || !backButton || !nextButton || !footerStatus) {
        console.error("One or more installer interface elements are missing.");
        return;
    }

    currentScreen = SCREENS[currentStep];

    renderSidebar();

    screenContent.innerHTML = "";

    const renderers = {
        welcome: renderWelcome,
        language: renderLanguage,
        license: renderLicense,
        integrity: renderIntegrity,
        region: renderRegion,
        keyboard: renderKeyboard,
        installation: renderInstallation,
        destination: renderDestination,
        preferences: renderPreferences,
        summary: renderSummary,
        installing: renderInstalling,
        complete: renderComplete
    };

    const renderer = renderers[currentScreen];

    if (!renderer) {
        console.error("Unknown installer screen:", currentScreen);
        return;
    }

    renderer();

    footerStatus.textContent = `${t("footer")} — ${currentStep + 1} / ${SCREENS.length}`;

    backButton.textContent = t("back");

    if (currentScreen !== "complete") {
        nextButton.textContent = t("next");
    }

    backButton.hidden = currentStep === 0 ||
        currentScreen === "installing" ||
        currentScreen === "complete";

    if (currentScreen !== "installing") {
        nextButton.hidden = false;
    }

    nextButton.disabled = false;

    if (currentScreen === "license") {
        const checkbox = document.getElementById("license-agreement");

        nextButton.disabled = !checkbox || !checkbox.checked;
    }

    if (currentScreen === "integrity") {
        nextButton.disabled =
            integrityChecking ||
            !integrityResult ||
            (!integrityResult.verified && !integrityAcknowledged);
    }

    if (currentScreen === "complete") {
        nextButton.textContent = t("finish");
        nextButton.hidden = false;
        nextButton.disabled = false;
    }

    saveSettings();
}

function nextStep() {
    if (currentScreen === "license") {
        const checkbox = document.getElementById("license-agreement");

        if (!checkbox || !checkbox.checked) {
            return;
        }

        window.EagleOSLicense.accept();
    }

    if (currentScreen === "integrity") {
        if (integrityChecking || !integrityResult) {
            return;
        }

        if (!integrityResult.verified && !integrityAcknowledged) {
            return;
        }
    }

    if (currentScreen === "complete") {
        return;
    }

    if (currentScreen === "summary") {
        currentStep = SCREENS.indexOf("installing");
        renderStep();
        return;
    }

    if (currentStep < SCREENS.length - 1) {
        currentStep++;
        renderStep();
    }
}

function previousStep() {
    if (currentStep <= 0 || installationRunning) {
        return;
    }

    currentStep--;

    renderStep();
}

function resetInstaller() {
    if (installationTimer) {
        clearInterval(installationTimer);
        installationTimer = null;
    }

    installationRunning = false;
    installationProgress = 0;

    integrityResult = null;
    integrityCheckPromise = null;
    integrityAcknowledged = false;
    integrityChecking = false;

    settings = cloneDefaults();
    currentStep = 0;

    saveSettings();

    if (window.EagleOSLicense) {
        window.EagleOSLicense.reset();
    }

    renderStep();
}

function initializeInstaller() {
    screenContent = document.getElementById("screen-content");
    backButton = document.getElementById("back-button");
    nextButton = document.getElementById("next-button");
    footerStatus = document.getElementById("footer-status");

    if (!screenContent || !backButton || !nextButton || !footerStatus) {
        console.error("EagleOS Setup could not initialize. Required HTML elements are missing.");
        return;
    }

    backButton.addEventListener("click", previousStep);
    nextButton.addEventListener("click", nextStep);

    renderStep();

    console.log("EagleOS Setup initialized.");
}

document.addEventListener("DOMContentLoaded", initializeInstaller);

window.EagleOSInstaller = {
    reset: resetInstaller,

    getSettings() {
        return JSON.parse(JSON.stringify(settings));
    },

    getCurrentScreen() {
        return currentScreen;
    },

    getScreens() {
        return [...SCREENS];
    }
};