#include "mainwindow.h"
#include "ui_mainwindow.h"

const QColor MainWindow::RED_LIGHT_OFF = QColor(96, 32, 32);
const QColor MainWindow::YELLOW_LIGHT_OFF = QColor(96, 96, 32);
const QColor MainWindow::GREEN_LIGHT_OFF = QColor(32, 96, 32);
const QColor MainWindow::RED_LIGHT_ON = QColor(255, 64, 64);
const QColor MainWindow::YELLOW_LIGHT_ON = QColor(223, 223, 64);
const QColor MainWindow::GREEN_LIGHT_ON = QColor(64, 255, 64);

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    qApp->installEventFilter(this);
    qRegisterMetaType<QVector<int> >("QVector<int>");

#ifdef Q_OS_LINUX
    //send_log_window_message("Running on Linux Desktop ", true, false);
    //serialPort = serialPortLinux;
    serial_port_prefix = "/dev/";
#endif

#if defined(_WIN32) || defined(WIN32) || defined (_WIN64) || defined (WIN64)
    //serialPort = serialPortWindows;
    serial_port_prefix = "";
#endif

#if Q_PROCESSOR_WORDSIZE == 4
    //qDebug() << "32-bit executable";
    //send_log_window_message("32-bit executable", false, true);
#elif Q_PROCESSOR_WORDSIZE == 8
    //qDebug() << "64-bit executable";
    //send_log_window_message("64-bit executable", false, true);
#endif

    ui->calibrationFilesTreeWidget->setHeaderLabel("Calibration Files");
    ui->calibrationDataTreeWidget->setHeaderLabel("Calibration Data");

    fileActions = new FileActions();
    configValues = &fileActions->ConfigValuesStruct;

    //fileActions->check_config_dir(configValues);
    configValues = fileActions->read_config_file(configValues);

    fileActions->read_protocols_file(configValues);
    qDebug() << "ECU protocols read";

    configValues->flash_protocol_selected_make = configValues->flash_protocol_make.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_mcu = configValues->flash_protocol_mcu.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_model = configValues->flash_protocol_model.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_version = configValues->flash_protocol_version.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_family = configValues->flash_protocol_family.at(configValues->flash_protocol_selected_id.toInt());
    configValues->flash_protocol_selected_description = configValues->flash_protocol_description.at(configValues->flash_protocol_selected_id.toInt());
    //configValues->flash_protocol_selected_flash_transport = configValues->flash_protocol_flash_transport.at(configValues->flash_protocol_selected_id.toInt());
    //configValues->flash_protocol_selected_log_transport = configValues->flash_protocol_log_transport.at(configValues->flash_protocol_selected_id.toInt());
    //configValues->flash_protocol_selected_log_protocol = configValues->flash_protocol_log_protocol.at(configValues->flash_protocol_selected_id.toInt());

    qDebug() << configValues->flash_protocol_selected_make;
    qDebug() << configValues->flash_protocol_selected_mcu;
    qDebug() << configValues->flash_protocol_selected_model;
    qDebug() << configValues->flash_protocol_selected_version;
    qDebug() << configValues->flash_protocol_selected_family;
    qDebug() << configValues->flash_protocol_selected_description;
    qDebug() << configValues->flash_protocol_selected_flash_transport;
    qDebug() << configValues->flash_protocol_selected_log_transport;
    qDebug() << configValues->flash_protocol_selected_log_protocol;
    QRect qrect = MainWindow::geometry();

    toolbar_item_size.setWidth(configValues->toolbar_iconsize.toInt());
    toolbar_item_size.setHeight(configValues->toolbar_iconsize.toInt());
    ui->toolBar->setIconSize(toolbar_item_size);

    if (configValues->window_width != "maximized" && configValues->window_height != "maximized")
        this->setGeometry(qrect.x(), qrect.y(), configValues->window_width.toInt(), configValues->window_height.toInt());
    else
        this->setWindowState(Qt::WindowMaximized);

    if (!configValues->romraider_definition_files.length() && !configValues->primary_definition_base.contains("ecuflash"))
        QMessageBox::warning(this, tr("Ecu definition file"), "No definition file(s), use definition manager at 'Edit' menu to choose file(s)");

    fileActions->create_ecuflash_def_id_list(configValues);
    fileActions->create_romraider_def_id_list(configValues);

    if (QDir(configValues->kernel_files_directory).exists()){
        QDir dir(configValues->kernel_files_directory);
        QStringList nameFilter("*.bin");
        QStringList txtFilesAndDirectories = dir.entryList(nameFilter);
        //qDebug() << txtFilesAndDirectories;
    }

    QSignalMapper *mapper = fileActions->read_menu_file(ui->menubar, ui->toolBar);
    connect(mapper, SIGNAL(mapped   (QString)), this, SLOT(menu_action_triggered(QString)));


/*
    for (int i = 0; i < configValues->calibration_files.count(); i++)
    {
        QString filename = configValues->calibration_files.at(i);
        bool result = false;
        //qDebug() << "Open file" << filename;
        //ecuCalDef[ecuCalDefIndex] = new FileActions::EcuCalDefStructure;
        result = open_calibration_file(filename);
        if (result)
        {
            configValues->calibration_files.removeAt(i);
            fileActions->saveConfigFile();
            i--;
        }
    }
    if(ecuCalDefIndex > 0)
    {
        const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
        emit ui->calibrationFilesTreeWidget->clicked(index);
    }
*/
    connect(ui->switchBoxWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_switch_values()));
    connect(ui->logBoxWidget, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_digital_values()));
    connect(ui->mdiArea, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(change_gauge_values()));
    //connect(ui->action_Preferences, SIGNAL(triggered()), this, SLOT(openPreferences()));
    connect(ui->calibrationFilesTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(calibration_files_treewidget_item_selected(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem *,int)), this, SLOT(calibration_data_treewidget_item_selected(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemExpanded(QTreeWidgetItem*)), this, SLOT(calibration_data_treewidget_item_expanded(QTreeWidgetItem*)));
    connect(ui->calibrationDataTreeWidget, SIGNAL(itemCollapsed(QTreeWidgetItem*)), this, SLOT(calibration_data_treewidget_item_collapsed(QTreeWidgetItem*)));
    connect(calibrationTreeWidget, SIGNAL(closeRom()), this, SLOT(close_calibration()));

    status_bar_connection_label->setMargin(5);
    //status_bar_connection_label->setStyleSheet("QLabel { background-color : red; color : white; }");
    set_status_bar_label(false, false, "");


    status_bar_ecu_label->setText("");
    status_bar_ecu_label->setMargin(5);
    status_bar_ecu_label->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    //status_bar_ecu_label->setStyleSheet("QLabel { background-color : red; color : white; }");

    QWidget* status_bar_spacer = new QWidget();
    status_bar_spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    statusBar()->addWidget(status_bar_connection_label);
    statusBar()->addWidget(status_bar_spacer);
    statusBar()->addPermanentWidget(status_bar_ecu_label);
    statusBar()->setSizeGripEnabled(true);

    ui->calibrationDataTreeWidget->resizeColumnToContents(0);
    ui->calibrationDataTreeWidget->resizeColumnToContents(1);
    ui->calibrationFilesTreeWidget->setMinimumHeight(125);
    ui->calibrationFilesTreeWidget->minimumHeight();
    ui->calibrationFilesTreeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->calibrationFilesTreeWidget, SIGNAL(customContextMenuRequested(QPoint)), SLOT(custom_menu_requested(QPoint)));

    //ui->splitter->setStretchFactor(2, 1);
    ui->splitter->setSizes(QList<int>({125, INT_MAX}));

    serial = new SerialPortActions();

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->toolBar->addWidget(spacer);

    ui->toolBar->addSeparator();

    QPushButton *select_protocol_button = new QPushButton();
    select_protocol_button->setText("Select vehicle");
    //car_make_button->setMargin(10);
    select_protocol_button->setFixedHeight(toolbar_item_size.height());
    ui->toolBar->addWidget(select_protocol_button);
    connect(select_protocol_button, SIGNAL(clicked(bool)), this, SLOT(select_protocol()));

    flash_transport_list = new QComboBox();
    flash_transport_list->setFixedHeight(toolbar_item_size.height());
    flash_transport_list->setObjectName("flash_transport_list");
    ui->toolBar->addWidget(flash_transport_list);

    ui->toolBar->addSeparator();

    QLabel *log_transport = new QLabel("Log:");
    log_transport->setMargin(10);
    ui->toolBar->addWidget(log_transport);

    log_transport_list = new QComboBox();
    log_transport_list->setFixedHeight(toolbar_item_size.height());
    log_transport_list->setObjectName("log_transport_list");
    ui->toolBar->addWidget(log_transport_list);

    flash_transports = create_flash_transports_list();
    log_transports = create_log_transports_list();
    connect(flash_transport_list, SIGNAL(currentIndexChanged(int)), this, SLOT(flash_transport_changed()));
    connect(log_transport_list, SIGNAL(currentIndexChanged(int)), this, SLOT(log_transport_changed()));

    //ui->toolBar->addSeparator();

    QLabel *log_select = new QLabel();
    log_select->setMargin(5);
    ui->toolBar->addWidget(log_select);

    QCheckBox *ecu_check_box = new QCheckBox("ECU");
    ecu_check_box->setCheckState(Qt::Checked);
    ui->toolBar->addWidget(ecu_check_box);
    QCheckBox *tcu_check_box = new QCheckBox("TCU");
    ui->toolBar->addWidget(tcu_check_box);

    ui->toolBar->addSeparator();

    QLabel *serial_port_select = new QLabel("Port:");
    serial_port_select->setMargin(10);
    ui->toolBar->addWidget(serial_port_select);

    serial_port_list = new QComboBox();
    serial_port_list->setFixedHeight(toolbar_item_size.height());
    serial_port_list->setFixedWidth(180);
    serial_port_list->setObjectName("serial_port_list");
    serial_ports = serial->check_serial_ports();
    for (int i = 0; i < serial_ports.length(); i++)
    {
        serial_port_list->addItem(serial_ports.at(i));
        if (configValues->serial_port == serial_ports.at(i).split(" - ").at(0))
            serial_port_list->setCurrentIndex(i);
    }
    ui->toolBar->addWidget(serial_port_list);

    QPushButton *refresh_serial_list = new QPushButton();
    refresh_serial_list->setIcon(QIcon(":/icons/view-refresh.png"));
    refresh_serial_list->setIconSize(toolbar_item_size);
    connect(refresh_serial_list, SIGNAL(clicked(bool)), this, SLOT(check_serial_ports()));
    ui->toolBar->addWidget(refresh_serial_list);

    logValues = &fileActions->LogValuesStruct;
    logValues = fileActions->read_logger_definition_file();
    logBoxes = new LogBox();

    if (logValues != NULL)
    {
        int switchBoxCount = 20;
        int logBoxCount = 12;

        update_logboxes(protocol);
    }

    serial_port = serial_port_prefix + configValues->serial_port;
    serial_port_baudrate = default_serial_port_baudrate;
    serial->serial_port_baudrate = serial_port_baudrate;
    serial->serial_port = serial_port;

    serial_poll_timer = new QTimer(this);
    serial_poll_timer->setInterval(serial_poll_timer_timeout);
    connect(serial_poll_timer, SIGNAL(timeout()), this, SLOT(open_serial_port()));
    serial_poll_timer->start();

    ssm_init_poll_timer = new QTimer(this);
    ssm_init_poll_timer->setInterval(ssm_init_poll_timer_timeout);
    connect(ssm_init_poll_timer, SIGNAL(timeout()), this, SLOT(ecu_init()));
    ssm_init_poll_timer->start();

    logging_poll_timer = new QTimer(this);
    logging_poll_timer->setInterval(logging_poll_timer_timeout);
    connect(logging_poll_timer, SIGNAL(timeout()), this, SLOT(log_ssm_values()));

    logparams_poll_timer = new QTimer(this);
    logparams_poll_timer->setInterval(logparams_poll_timer_timeout);
    connect(logparams_poll_timer, SIGNAL(timeout()), this, SLOT(read_log_serial_data()));

    log_speed_timer = new QElapsedTimer();
    log_file_timer = new QElapsedTimer();

    if(ecuCalDefIndex > 0)
    {
        const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
        emit ui->calibrationFilesTreeWidget->clicked(index);
    }

    emit log_transport_list->currentIndexChanged(log_transport_list->currentIndex());

    status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");

    set_flash_arrow_state();

}

MainWindow::~MainWindow()
{
    if (logging_state)
    {
        logging_state = false;
        log_params_request_started = false;
        log_ssm_values();
        delay(200);
    }
    ssm_init_poll_timer->stop();
    serial_poll_timer->stop();
    delete ui;
}

void MainWindow::SetComboBoxItemEnabled(QComboBox * comboBox, int index, bool enabled)
{
    auto * model = qobject_cast<QStandardItemModel*>(comboBox->model());
    assert(model);
    if(!model) return;

    auto * item = model->item(index);
    assert(item);
    if(!item) return;
    item->setEnabled(enabled);
}

QStringList MainWindow::create_flash_transports_list()
{
    QStringList flash_protocols;

    flash_protocols.append(configValues->flash_protocol_flash_transport.at(configValues->flash_protocol_selected_id.toInt()).split(","));

    flash_transport_list->clear();
    for (int i = 0; i < flash_protocols.length(); i++){
        flash_transport_list->addItem(flash_protocols.at(i));
        if (configValues->flash_protocol_selected_flash_transport == flash_protocols.at(i))
            flash_transport_list->setCurrentIndex(i);
    }
    return flash_protocols;
}

QStringList MainWindow::create_log_transports_list()
{
    QStringList log_transports;

    log_transports.append(configValues->flash_protocol_log_transport.at(configValues->flash_protocol_selected_id.toInt()).split(","));

    log_transport_list->clear();
    for (int i = 0; i < log_transports.length(); i++){
        log_transport_list->addItem(log_transports.at(i));
        if (configValues->flash_protocol_selected_flash_transport == log_transports.at(i))
        {
            log_transport_list->setCurrentIndex(i);
            protocol = "SSM";
        }
    }

    //if (car_model_list->currentText() == "Subaru")
        //protocol = "SSM";

    return log_transports;
}

QString MainWindow::check_kernel(QString flash_method)
{
    QString kernel;
    QString prefix = configValues->kernel_files_directory;

    if (prefix.at(prefix.length() - 1) != "/")
        prefix.append("/");

    if (flash_method.startsWith("sub_mc68hc16y5_02"))
        kernel = prefix + "ssmk_HC16.bin";
    else if (flash_method.startsWith("sub_mc68hc16y5_04"))
        kernel = prefix + "ssmk_HC16.bin";
    else if (flash_method.startsWith("sub_sh7055_02"))
    {
        if (serial->is_can_connection)
            kernel = prefix + "ssmk_CAN_SH7055.bin";
        else
            kernel = prefix + "ssmk_SH7055.bin";
    }
    else if (flash_method.startsWith("sub_sh7055_04"))
    {
        if (serial->is_can_connection)
            kernel = prefix + "ssmk_CAN_SH7055.bin";
        else
            kernel = prefix + "ssmk_SH7055.bin";
    }
    else if (flash_method.startsWith("sub_sh7058_can_diesel"))
        if (serial->is_can_connection)
            kernel = prefix + "ssmk_CAN_SH7058.bin";
        else
            kernel = prefix + "ssmk_CAN_SH7058d.bin";
    else if (flash_method.startsWith("sub_sh7058_can"))
        kernel = prefix + "ssmk_CAN_SH7058.bin";
    else if (flash_method.startsWith("sub_sh7058"))
    {
        if (serial->is_can_connection)
            kernel = prefix + "ssmk_CAN_SH7058.bin";
        else
            kernel = prefix + "ssmk_SH7058.bin";
    }
    else if (flash_method.startsWith("sub_sh7055_denso_can"))
        kernel = prefix + "ssmk_CAN_SH7055.bin";
    else if (flash_method.startsWith("sub_sh7058_denso_can"))
        kernel = prefix + "ssmk_CAN_SH7058.bin";
    else if (flash_method.startsWith("sub_sh7058s_denso_can"))
        kernel = prefix + "ssmk_CAN_SH7058.bin";

    else
        kernel = "N/A";

    return kernel;
}

void MainWindow::select_protocol()
{
    //qDebug() << "Select protocol";
    ProtocolSelect *protocolSelect = new ProtocolSelect(configValues);
    connect(protocolSelect, SIGNAL(finished (int)), this, SLOT(select_protocol_finished(int)));
    protocolSelect->exec();

    //qDebug() << "Selected protocol:" << configValues->flash_protocol_selected_family;
    //status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");

}


void MainWindow::select_protocol_finished(int result)
{
   if(result == QDialog::Accepted)
   {
        create_flash_transports_list();
        create_log_transports_list();
        fileActions->save_config_file(configValues);

        set_flash_arrow_state();
   }
   else
   {
       //qDebug() << "Dialog is rejected";
   }

   status_bar_ecu_label->setText(configValues->flash_protocol_selected_description + " ");
}

void MainWindow::set_flash_arrow_state()
{
    QList<QMenu*> menus = ui->menubar->findChildren<QMenu*>();
    foreach (QMenu *menu, menus) {
        foreach (QAction *action, menu->actions()) {
            if (action->isSeparator()) {

            } else if (action->menu()) {

            } else {
                if (action->text() == "Read from ecu")
                {
                    if (configValues->flash_protocol_read.at(configValues->flash_protocol_selected_id.toInt()) == "yes")
                        action->setEnabled(true);
                    else
                        action->setEnabled(false);
                }
                if (action->text() == "Test write to ecu")
                {
                    if (configValues->flash_protocol_write.at(configValues->flash_protocol_selected_id.toInt()) == "yes")
                        action->setEnabled(true);
                    else
                        action->setEnabled(false);
                }
                if (action->text() == "Write to ecu")
                {
                    if (configValues->flash_protocol_write.at(configValues->flash_protocol_selected_id.toInt()) == "yes")
                        action->setEnabled(true);
                    else
                        action->setEnabled(false);
                }

            }
        }
    }
}

void MainWindow::log_transport_changed()
{
    //qDebug() << "Change log transport";
    QComboBox *log_transport_list = ui->toolBar->findChild<QComboBox*>("log_transport_list");

    serial->is_can_connection = false;
    serial->is_iso15765_connection = false;
    if (log_transport_list->currentText() == "CAN")
    {
        serial->is_can_connection = true;
        serial->is_iso15765_connection = false;
        serial->is_29_bit_id = false;
        serial->can_speed = "500000";
    }
    else if (log_transport_list->currentText() == "iso15765")
    {
        serial->is_can_connection = false;
        serial->is_iso15765_connection = true;
        serial->is_29_bit_id = true;
        serial->can_speed = "500000";
    }
    else if (log_transport_list->currentText() == "K-Line")
    {
        if (configValues->flash_protocol_selected_log_protocol == "SSM")
            serial->change_port_speed("4800");
    }

    protocol = configValues->flash_protocol_selected_log_protocol;
    configValues->flash_protocol_selected_log_transport = log_transport_list->currentText();
    fileActions->save_config_file(configValues);

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    ssm_init_poll_timer->start();
}

void MainWindow::flash_transport_changed()
{
    //qDebug() << "Change flash transport";
    QComboBox *flash_transport_list = ui->toolBar->findChild<QComboBox*>("flash_transport_list");

    configValues->flash_protocol_selected_flash_transport = flash_transport_list->currentText();
    fileActions->save_config_file(configValues);
}

void MainWindow::check_serial_ports()
{
    QComboBox *serial_port_list = ui->toolBar->findChild<QComboBox*>("serial_port_list");
    QString prev_serial_port = serial_port_list->currentText();
    int index = 0;

    serial_ports = serial->check_serial_ports();
    serial_port_list->clear();

    for (int i = 0; i < serial_ports.length(); i++)
    {
        serial_port_list->addItem(serial_ports.at(i));
        if (prev_serial_port == serial_ports.at(i))
            serial_port_list->setCurrentIndex(index);
        index++;
    }
}

void MainWindow::open_serial_port()
{
    QStringList serial_port;

    if (serial_ports.length() > 0)
    {
        serial_port.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(0));
        serial_port.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(1));

        serial->serial_port_list = serial_port;
        QString opened_serial_port = serial->open_serial_port();
        if (opened_serial_port != NULL)
        {
            if (opened_serial_port != previous_serial_port)
            {
                ecuid.clear();
                ecu_init_complete = false;
            }
            //qDebug() << "Serial port" << opened_serial_port << "opened" << previous_serial_port;
            previous_serial_port = opened_serial_port;
            configValues->serial_port = serial_port.at(0);
            fileActions->save_config_file(configValues);
            if (ecuid == "")
                set_status_bar_label(true, false, "");
            else
                set_status_bar_label(true, true, ecuid);
        }
        else
        {
            set_status_bar_label(false, false, "");
            ecu_init_complete = false;
        }
    }
}

int MainWindow::start_ecu_operations(QString cmd_type)
{
    int rom_number = 0;

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
    if (!selectedItem && cmd_type != "read")
    {
        QMessageBox::warning(this, tr("Write ROM"), "No file selected!");
        return 0;
    }

    QComboBox *serial_port_list = ui->toolBar->findChild<QComboBox*>("serial_port_list");
    if (serial_port_list->currentText() == "")
    {
        qDebug() << "No serial port selected!";
        QMessageBox::warning(this, tr("Serial port"), "No serial port selected!");
        return 0;
    }
    rom_number = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    // Stop serial timers
    serial_poll_timer->stop();
    ssm_init_poll_timer->stop();
    logging_poll_timer->stop();

    serial->serial_port_list.clear();
    serial->serial_port_list.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(0));
    serial->serial_port_list.append(serial_ports.at(serial_port_list->currentIndex()).split(" - ").at(1));

    if (configValues->flash_protocol_selected_make == "Subaru")
    {
        if (flash_transport_list->currentText() == "CAN")
        {
            serial->is_can_connection = true;
            serial->is_iso15765_connection = false;
            serial->is_29_bit_id = true;
            serial->can_speed = "500000";
        }
        else if (flash_transport_list->currentText() == "iso15765")
        {
            serial->is_can_connection = false;
            serial->is_iso15765_connection = true;
            serial->is_29_bit_id = false;
            serial->can_speed = "500000";
        }

        serial->reset_connection();
        ecuid.clear();
        ecu_init_complete = false;
        serial->add_iso14230_header = false;
        //open_serial_port();

        if (cmd_type == "test_write" || cmd_type == "write")
        {
            fileActions->checksum_correction(ecuCalDef[rom_number]);

            ecuCalDef[rom_number]->RomInfo.replace(fileActions->FlashMethod, configValues->flash_protocol_selected_family);
            ecuCalDef[rom_number]->Kernel = configValues->flash_protocol_kernel.at(configValues->flash_protocol_selected_id.toInt()); //check_kernel(ecuCalDef[rom_number]->RomInfo.at(FlashMethod));
            ecuCalDef[rom_number]->KernelStartAddr = configValues->flash_protocol_kernel_addr.at(configValues->flash_protocol_selected_id.toInt());
            ecuCalDef[rom_number]->McuType = configValues->flash_protocol_selected_mcu;
        }
        else
        {
            rom_number = ecuCalDefIndex;
            ecuCalDef[rom_number] = new FileActions::EcuCalDefStructure;
            while (ecuCalDef[rom_number]->RomInfo.length() < fileActions->RomInfoStrings.length()){
                ecuCalDef[rom_number]->RomInfo.append(" ");
            }
            ecuCalDef[rom_number]->RomInfo.replace(fileActions->FlashMethod, configValues->flash_protocol_selected_family);
            ecuCalDef[rom_number]->FlashMethod = configValues->flash_protocol_selected_family;
            ecuCalDef[rom_number]->Kernel = configValues->flash_protocol_kernel.at(configValues->flash_protocol_selected_id.toInt()); //check_kernel(ecuCalDef[rom_number]->RomInfo.at(fileActions->FlashMethod));
            ecuCalDef[rom_number]->KernelStartAddr = configValues->flash_protocol_kernel_addr.at(configValues->flash_protocol_selected_id.toInt());
            ecuCalDef[rom_number]->McuType = configValues->flash_protocol_selected_mcu;
        }

        qDebug() << "Family to use:" << configValues->flash_protocol_selected_family;

        if (configValues->flash_protocol_selected_flash_transport == "CAN" && !configValues->flash_protocol_selected_family.endsWith("sub_denso_can_recovery"))
        {
            if (ecuCalDef[rom_number]->McuType == "SH7055")
                ecuCalDef[rom_number]->FlashMethod = "sh7055_denso_can";
            else if (ecuCalDef[rom_number]->McuType.startsWith("SH7058")){
                ecuCalDef[rom_number]->FlashMethod = "sh7058_denso_can";
                ecuCalDef[rom_number]->McuType = "SH7058";
            }
            flashEcuSubaruDensoSH705xCan = new FlashEcuSubaruDensoSH705xCan(serial, ecuCalDef[rom_number], cmd_type, this);
        }
        else if (configValues->flash_protocol_selected_family.startsWith("sub_mc68hc16y5_02"))
            flashEcuSubaruDensoMC68HC16Y5_02 = new FlashEcuSubaruDensoMC68HC16Y5_02(serial, ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_mc68hc16y5_04"))
            flashEcuSubaruDensoMC68HC16Y5_02 = new FlashEcuSubaruDensoMC68HC16Y5_02(serial, ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_sh7055_02"))
            flashEcuSubaruDensoSH7055_02 = new FlashEcuSubaruDensoSH7055_02(serial, ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_sh7055_04"))
            flashEcuSubaruDensoSH7055_04 = new FlashEcuSubaruDensoSH7055_04(serial, ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_sh7058"))
            flashEcuSubaruDensoSH7055_04 = new FlashEcuSubaruDensoSH7055_04(serial, ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_sh7058_can_diesel"))
            flashEcuSubaruDensoSH7058CanDiesel = new FlashEcuSubaruDensoSH7058CanDiesel(serial, ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_sh7058_can"))
            flashEcuSubaruDensoSH7058Can = new FlashEcuSubaruDensoSH7058Can(serial, ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_unisia_jecs"))
            flashEcuSubaruUnisiaJecs = new FlashEcuSubaruUnisiaJecs(serial,ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_hitachi_02"))
            flashEcuSubaruHitachiM32R_02 = new FlashEcuSubaruHitachiM32R_02(serial,ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_hitachi_06"))
            flashEcuSubaruHitachiM32R_06 = new FlashEcuSubaruHitachiM32R_06(serial,ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.startsWith("sub_hitachi_can"))
            flashEcuSubaruHitachiCan = new FlashEcuSubaruHitachiCan(serial,ecuCalDef[rom_number], cmd_type, this);
        else if (configValues->flash_protocol_selected_family.endsWith("sub_denso_can_recovery"))
        {
            if (ecuCalDef[rom_number]->McuType.startsWith("SH7058"))
                ecuCalDef[rom_number]->McuType = "SH7058";
            flashEcuSubaruDensoSH705xCan = new FlashEcuSubaruDensoSH705xCan(serial, ecuCalDef[rom_number], cmd_type, this);
        }
        else
            ecuOperationsSubaru = new EcuOperationsSubaru(serial, ecuCalDef[rom_number], cmd_type, this);

        if (cmd_type == "read")
        {
            if (ecuCalDef[rom_number]->FullRomData.length())
            {
                //qDebug() << "Checking definitions, please wait...";
                fileActions->open_subaru_rom_file(ecuCalDef[rom_number], ecuCalDef[rom_number]->FullFileName);
                //if(ecuCalDef[ecuCalDefIndex]->RomId != NULL)
                //{
                    //qDebug() << "Building treewidget, please wait...";
                    calibrationTreeWidget->buildCalibrationFilesTree(rom_number, ui->calibrationFilesTreeWidget, ecuCalDef[rom_number]);
                    calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[rom_number]);

                //}
                ecuCalDefIndex++;
                save_calibration_file_as();
            }
        }
    }
    else if (configValues->flash_protocol_selected_make == "Mercedes")
    {
        qDebug() << "Testing MB EDC16 ECU";

        // Reset all comms
        serial->reset_connection();
        ecuid.clear();
        ecu_init_complete = false;

        // For CAN comms
        if (flash_transport_list->currentText() == "CAN")
            serial->is_can_connection = true;
        if (flash_transport_list->currentText() == "iso15765")
            serial->is_iso15765_connection = true;

        if (serial->is_can_connection || serial->is_iso15765_connection)
        {
            serial->is_29_bit_id = false;
            serial->can_speed = "500000";
            open_serial_port();
        }
        // For K-Line comms
        else
        {
            serial->add_iso14230_header = true;
            open_serial_port();
            serial->change_port_speed("10400");
        }

        ecuOperationsRenault = new EcuOperationsMercedes(serial, ecuCalDef[ecuCalDefIndex], cmd_type);
    }

    serial->reset_connection();
    ecuid.clear();
    ecu_init_complete = false;
    serial->is_29_bit_id = false;
    serial->add_iso14230_header = false;
    serial->is_can_connection = false;
    serial->is_iso15765_connection = false;
    serial->serial_port_baudrate = "4800";
    emit log_transport_list->currentIndexChanged(log_transport_list->currentIndex());
    //if(configValues->flash_method != "subarucan" && configValues->flash_method != "subarucan_iso")
    serial_poll_timer->start();
    ssm_init_poll_timer->start();
    serial->change_port_speed("4800");

    return 0;
}

int MainWindow::start_manual_ecu_operations()
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    EcuManualOperations *ecuManualOperations = new EcuManualOperations(serial, ecuCalDef[romNumber]);

    serial->change_port_speed("4800");

    return 0;
}

void MainWindow::custom_menu_requested(QPoint pos)
{
    QModelIndex index = ui->calibrationFilesTreeWidget->indexAt(pos);
    emit ui->calibrationFilesTreeWidget->clicked(index);

    QMenu *menu = new QMenu(this);
    menu->addAction("Sync with ECU", this, SLOT(syncCalWithEcu()));
    menu->addAction("Close", this, SLOT(close_calibration()));
    menu->popup(ui->calibrationFilesTreeWidget->viewport()->mapToGlobal(pos));

}

bool MainWindow::open_calibration_file(QString filename)
{
    ecuCalDef[ecuCalDefIndex] = new FileActions::EcuCalDefStructure;

    while (ecuCalDef[ecuCalDefIndex]->RomInfo.length() < fileActions->RomInfoStrings.length())
        ecuCalDef[ecuCalDefIndex]->RomInfo.append(" ");

    ecuCalDef[ecuCalDefIndex] = fileActions->open_subaru_rom_file(ecuCalDef[ecuCalDefIndex], filename);

    if(ecuCalDef[ecuCalDefIndex] != NULL)
    {
        calibrationTreeWidget->buildCalibrationFilesTree(ecuCalDefIndex, ui->calibrationFilesTreeWidget, ecuCalDef[ecuCalDefIndex]);
        calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[ecuCalDefIndex]);

        ecuCalDefIndex++;
    }
    else
    {
        //qDebug() << "Failed!!";
        ecuCalDef[ecuCalDefIndex] = {};
        return 1;
    }
    return 0;
}

void MainWindow::save_calibration_file()
{
    if (ecuCalDefIndex == 0){
        QMessageBox::information(this, tr("Calibration file"), "No calibration to save!");
        return;
    }

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(2).toInt();

    fileActions->save_subaru_rom_file(ecuCalDef[romNumber], ecuCalDef[romNumber]->FullFileName);

}

void MainWindow::save_calibration_file_as()
{
    if (ecuCalDefIndex == 0){
        QMessageBox::information(this, tr("Calibration file"), "No calibration to save!");
        return;
    }

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    qDebug() << "Save as: Check selected ROM number";
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(2).toInt();

    QString filename = "";

    QFileDialog saveDialog;
    saveDialog.setDefaultSuffix("bin");
    qDebug() << "Save as: Check if OEM ECU file";

    filename = QFileDialog::getSaveFileName(this, tr("Save calibration file"), configValues->calibration_files_base_directory, tr("Calibration file (*.bin)"));

    if (filename.isEmpty()){
        ui->calibrationFilesTreeWidget->selectedItems().at(0)->setText(0, ecuCalDef[romNumber]->FileName);
        QMessageBox::information(this, tr("Calibration file"), "No file name selected");
        return;
    }

    if(!filename.endsWith(QString(".bin")))
         filename.append(QString(".bin"));

    fileActions->save_subaru_rom_file(ecuCalDef[romNumber], filename);
    ui->calibrationFilesTreeWidget->selectedItems().at(0)->setText(0, ecuCalDef[romNumber]->FileName);
}

void MainWindow::selectable_combobox_item_changed(QString item)
{
    int mapRomNumber = 0;
    int mapNumber = 0;

    //bool ok = false;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        mapRomNumber = mapWindowString.at(0).toInt();
        mapNumber = mapWindowString.at(1).toInt();

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QStringList selectionsNameList = ecuCalDef[mapRomNumber]->SelectionsNameList.at(mapNumber).split(",");
            QStringList selectionsValueList = ecuCalDef[mapRomNumber]->SelectionsValueList.at(mapNumber).split(",");

            for (int j = 0; j < selectionsNameList.length(); j++){
                if (selectionsNameList.at(j) == item)
                {
                    ecuCalDef[mapRomNumber]->MapData.replace(mapNumber, selectionsValueList.at(j));
                }
            }
        }
    }
}

void MainWindow::checkbox_state_changed(int state)
{

    bool bStatus;

    int map_rom_number = 0;
    int map_number = 0;

    QMdiSubWindow* w = ui->mdiArea->activeSubWindow();
    if (w)
    {
        QStringList mapWindowString = w->objectName().split(",");
        map_rom_number = mapWindowString.at(0).toInt();
        map_number = mapWindowString.at(1).toInt();

        QTableWidget* mapTableWidget = w->findChild<QTableWidget*>(w->objectName());
        if (mapTableWidget)
        {
            QStringList switch_states = ecuCalDef[map_rom_number]->StateList.at(map_number).split(",");
            int switch_states_length = (switch_states.length() - 1);
            for (int i = 0; i < switch_states_length; i += 2)
            {
                QStringList switch_data = switch_states.at(i + 1).split(" ");
                uint32_t byte_address = ecuCalDef[map_rom_number]->AddressList.at(map_number).toUInt(&bStatus, 16);
                if (ecuCalDef[map_rom_number]->RomInfo.at(FlashMethod) == "wrx02" && ecuCalDef[map_rom_number]->FileSize < (170 * 1024) && byte_address > 0x27FFF)
                    byte_address -= 0x8000;
                if ((switch_states.at(i) == "off" || switch_states.at(i) == "disabled") && state == 0)
                {
                    for (int j = 0; j < switch_data.length(); j++)
                    {
                        ecuCalDef[map_rom_number]->FullRomData[byte_address + j] = (uint8_t)switch_data.at(j).toUInt();
                    }
                }
                if ((switch_states.at(i) == "on" || switch_states.at(i) == "enabled") && state == 2)
                {
                    for (int j = 0; j < switch_data.length(); j++)
                    {
                        ecuCalDef[map_rom_number]->FullRomData[byte_address + j] = (uint8_t)switch_data.at(j).toUInt();
                    }
                }
            }
        }
    }

}

void MainWindow::calibration_files_treewidget_item_selected(QTreeWidgetItem* item)
{
    QList<QTreeWidgetItem *> itemList;
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    for( int i = 0; i < ui->calibrationFilesTreeWidget->topLevelItemCount(); i++ )
    {
        QTreeWidgetItem *item = ui->calibrationFilesTreeWidget->topLevelItem(i);
        item->setCheckState(0, Qt::Unchecked);
    }

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);

    selectedItem->setCheckState(0, Qt::Checked);

    QString romId = selectedItem->text(1);

    calibrationTreeWidget->buildCalibrationDataTree(ui->calibrationDataTreeWidget, ecuCalDef[romNumber]);
/*
    QComboBox *flash_method_list = ui->toolBar->findChild<QComboBox*>("flash_method_list");
    for (int i = 0; i < flash_method_list->count(); i++)
    {
        if(ecuCalDef[romNumber]->RomInfo.at(FlashMethod) == flash_method_list->itemText(i))
        {
            flash_method_list->setCurrentIndex(i);
        }
    }
    */
}

void MainWindow::calibration_data_treewidget_item_selected(QTreeWidgetItem* item)
{
    const QModelIndex index = ui->calibrationDataTreeWidget->selectionModel()->currentIndex();
    QString selectedText = index.data(Qt::DisplayRole).toString();
    int hierarchyLevel=1;
    QModelIndex seekRoot = index;
    QString selectedRom;

    selectedText = item->text(0);

    while(seekRoot.parent() != QModelIndex())
    {
        seekRoot = seekRoot.parent();
        hierarchyLevel++;
    }

    if (ui->calibrationDataTreeWidget->indexOfTopLevelItem(item) > -1){
        hierarchyLevel = 1;
    }
    else if (ui->calibrationDataTreeWidget->indexOfTopLevelItem(item->parent()) > -1){
        hierarchyLevel = 2;

        QTreeWidgetItem *selectedFilesTreeItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
        QTreeWidgetItem *selectedDataTreeItem = item;
        int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedFilesTreeItem);
        int mapIndex = selectedDataTreeItem->text(1).toInt();
        int romIndex = selectedFilesTreeItem->text(2).toInt();

        for (int i = 0; i < ecuCalDef[romNumber]->NameList.count(); i++)
        {
            if (ecuCalDef[romNumber]->NameList.at(i) == selectedText && i == mapIndex)
            {
                if (ecuCalDef[romNumber]->VisibleList.at(i) == "1")
                {
                    int map_index = 0;
                    QList<QMdiSubWindow *> list = ui->mdiArea->findChildren<QMdiSubWindow *>();
                    foreach(QMdiSubWindow *w, list)
                    {
                        //qDebug() << map_index << w->objectName();
                        map_index++;
                        if (w->objectName().startsWith(QString::number(romIndex) + "," + QString::number(i) + "," + ecuCalDef[romNumber]->NameList.at(i)))
                        {
                            if (w->objectName() == ui->mdiArea->activeSubWindow()->objectName())
                            {
                                ui->mdiArea->removeSubWindow(w);
                                ecuCalDef[romNumber]->VisibleList.replace(i, "0");
                                item->setCheckState(0, Qt::Unchecked);
                            }
                            else
                            {
                                w->setFocus();
                                item->setCheckState(0, Qt::Checked);

                            }
                        }
                    }
                }
                else
                {
                    ecuCalDef[romNumber]->VisibleList.replace(i, "1");
                    item->setCheckState(0, Qt::Checked);

                    CalibrationMaps *calibrationMaps = new CalibrationMaps(ecuCalDef[romNumber], romIndex, i, ui->mdiArea->contentsRect());
                    QMdiSubWindow *subWindow = ui->mdiArea->addSubWindow(calibrationMaps);
                    if (subWindow)
                    {
                        subWindow->setAttribute(Qt::WA_DeleteOnClose, true);
                        subWindow->setObjectName(calibrationMaps->objectName());
                        subWindow->show();
                        subWindow->adjustSize();
                        subWindow->move(0,0);
                        //subWindow->setFixedWidth(subWindow->width());
                        //subWindow->setFixedHeight(subWindow->height());

                        connect(calibrationMaps, SIGNAL(selectable_combobox_item_changed(QString)), this, SLOT(selectable_combobox_item_changed(QString)));
                        connect(calibrationMaps, SIGNAL(checkbox_state_changed(int)), this, SLOT(checkbox_state_changed(int)));
                        connect(subWindow, SIGNAL(destroyed(QObject*)), this, SLOT(close_calibration_map(QObject*)));
                    }
                }
            }
        }
    }
}

void MainWindow::calibration_data_treewidget_item_expanded(QTreeWidgetItem* item)
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int itemIndex = ui->calibrationDataTreeWidget->indexOfTopLevelItem(item);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    QString categoryName = ui->calibrationDataTreeWidget->topLevelItem(itemIndex)->text(0);

    calibrationTreeWidget->calibrationDataTreeWidgetItemExpanded(ecuCalDef[romNumber], categoryName);
}

void MainWindow::calibration_data_treewidget_item_collapsed(QTreeWidgetItem* item)
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int itemIndex = ui->calibrationDataTreeWidget->indexOfTopLevelItem(item);
    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    QString categoryName = ui->calibrationDataTreeWidget->topLevelItem(itemIndex)->text(0);

    calibrationTreeWidget->calibrationDataTreeWidgetItemCollapsed(ecuCalDef[romNumber], categoryName);
}

void MainWindow::close_calibration()
{
    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);

    int romNumber = ui->calibrationFilesTreeWidget->indexOfTopLevelItem(selectedItem);
    int romIndex = ui->calibrationFilesTreeWidget->selectedItems().at(0)->text(2).toInt();

    for (int i = 0; i < ui->calibrationDataTreeWidget->topLevelItemCount(); i++)
    {
        for (int j = 0; j < ui->calibrationDataTreeWidget->topLevelItem(i)->childCount(); j++)
        {
            for (int k = 0; k < ui->mdiArea->subWindowList().count(); k++)
            {
                int mapNumber = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j)->text(1).toInt();
                QString mapName = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j)->text(0);
                QString mapType = ecuCalDef[romNumber]->TypeList.at(mapNumber);

                QString calMapWindowName = QString::number(romIndex) + "," + QString::number(mapNumber) + "," + mapName;
                QMdiSubWindow *w = ui->mdiArea->subWindowList().at(k);
                if (w->objectName().startsWith(calMapWindowName))
                {
                    ui->mdiArea->removeSubWindow(w);
                }

            }

        }
    }
    delete ui->calibrationFilesTreeWidget->takeTopLevelItem(romNumber);

    ecuCalDefIndex--;
    for (int i = romNumber; i < ecuCalDefIndex; i++)
    {
        if (ecuCalDefIndex > romNumber)
        {
            ecuCalDef[i] = ecuCalDef[i + 1];
            int rom_index = ui->calibrationFilesTreeWidget->topLevelItem(i)->text(2).toInt();
            ui->calibrationFilesTreeWidget->topLevelItem(i)->setText(2, QString::number(rom_index - 1));
        }
    }
    ecuCalDef[ecuCalDefIndex] = {};

    if (ui->calibrationFilesTreeWidget->topLevelItemCount() > 0)
    {
        for (int i = 0; i < ui->calibrationFilesTreeWidget->topLevelItemCount(); i++)
        {
            ui->calibrationFilesTreeWidget->topLevelItem(i)->setSelected(false);
        }
        int activeRom = 0;
        if (romNumber > 0)
            activeRom = romNumber - 1;
        QTreeWidgetItem *currentItem = ui->calibrationFilesTreeWidget->topLevelItem(activeRom);
        currentItem->setSelected(true);
        const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
        emit ui->calibrationFilesTreeWidget->clicked(index);
    }
    else
    {
        for (int i = ui->calibrationDataTreeWidget->topLevelItemCount(); i > 0; i--)
        {
            delete ui->calibrationDataTreeWidget->takeTopLevelItem(0);
        }
    }
    //configValues->calibration_files.removeAt(romNumber);
    //fileActions->save_config_file(configValues);
}

void MainWindow::close_calibration_map(QObject* obj)
{
    QStringList mapWindowString = obj->objectName().split(",");
    int mapRomNumber = mapWindowString.at(0).toInt();
    QString mapName = mapWindowString.at(2);

    for (int i = 0; i < ui->calibrationFilesTreeWidget->topLevelItemCount(); i++)
    {
        ui->calibrationFilesTreeWidget->topLevelItem(i)->setSelected(false);
    }
    QTreeWidgetItem *currentItem = ui->calibrationFilesTreeWidget->topLevelItem(mapRomNumber);
    currentItem->setSelected(true);
    const QModelIndex index = ui->calibrationFilesTreeWidget->selectionModel()->currentIndex();
    emit ui->calibrationFilesTreeWidget->clicked(index);

    QTreeWidgetItem *selectedItem = ui->calibrationFilesTreeWidget->selectedItems().at(0);
    int romIndex = selectedItem->text(2).toInt();

    QTreeWidgetItem *item;
    for(int i = 0 ; i < ui->calibrationDataTreeWidget->topLevelItemCount(); i++)
    {
        for(int j = 0 ; j < ui->calibrationDataTreeWidget->topLevelItem(i)->childCount(); j++)
        {
            item = ui->calibrationDataTreeWidget->topLevelItem(i)->child(j);
            if (item->text(0) == mapName)
            {
                if (mapRomNumber == romIndex)
                    item->setCheckState(0, Qt::Unchecked);

                for (int i = 0; i < ecuCalDef[mapRomNumber]->NameList.count(); i++)
                {
                    if (ecuCalDef[mapRomNumber]->NameList.at(i) == mapName)
                    {
                        ecuCalDef[mapRomNumber]->VisibleList.replace(i, "0");
                    }
                }

            }
        }
    }

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    close_app();
}

void MainWindow::close_app()
{
    FileActions::ConfigValuesStructure *configValues = &fileActions->ConfigValuesStruct;

    qApp->exit();
}

void MainWindow::change_gauge_values()
{
    change_log_values(0, protocol);
    //if (ecu_init_complete)
    //    fileActions->read_logger_conf(logValues, ecuid, true);
}

void MainWindow::change_digital_values()
{
    change_log_values(1, protocol);
}

void MainWindow::change_switch_values()
{
    change_log_values(2, protocol);
}

void MainWindow::update_logboxes(QString protocol)
{
    int switchBoxCount = 20;
    int logBoxCount = 12;

    qDebug() << "Update logboxes with protocol:" << protocol;

    while(!ui->switchBoxLayout->isEmpty()) {
        QWidget *wg = ui->switchBoxLayout->takeAt(0)->widget();
        delete wg;
    }
    while(!ui->logBoxLayout->isEmpty()) {
        QWidget *wg = ui->logBoxLayout->takeAt(0)->widget();
        delete wg;
    }

    for (int i = 0; i < logValues->lower_panel_switch_id.count(); i++)
    {
        for (int j = 0; j < logValues->log_switch_id.length(); j++)
        {
            if (logValues->lower_panel_switch_id.at(i) == logValues->log_switch_id.at(j) && logValues->log_switch_protocol.at(j) == protocol)
            {
                QGroupBox *switchBox = logBoxes->drawLogBoxes("switch", i, switchBoxCount, logValues->log_switch_name.at(j), logValues->log_switch_name.at(j), logValues->log_switch_state.at(j));
                switchBox->setAttribute(Qt::WA_TransparentForMouseEvents);
                ui->switchBoxLayout->addWidget(switchBox);
            }
        }
    }
    for (int i = 0; i < logValues->lower_panel_log_value_id.count(); i++)
    {
        for (int j = 0; j < logValues->log_value_id.length(); j++)
        {
            if (logValues->lower_panel_log_value_id.at(i) == logValues->log_value_id.at(j) && logValues->log_value_protocol.at(j) == protocol)
            {
                QStringList value_unit = logValues->log_value_units.at(j).split(",");
                QGroupBox *logBox = logBoxes->drawLogBoxes("log", i, logBoxCount, logValues->log_value_name.at(j), value_unit.at(1), logValues->log_value.at(j));
                logBox->setAttribute(Qt::WA_TransparentForMouseEvents);
                ui->logBoxLayout->addWidget(logBox);
            }
        }
    }
}

void MainWindow::update_logbox_values(QString protocol)
{
    int index = 0;
    int decimal_count;
    QString warningMin;
    QString warningMax;
    QString labelText;
    QString label;
    QString unit;
    double log_value;
    bool warning = false;

    QScreen *screen = QGuiApplication::primaryScreen();
    QRect size = screen->geometry();

    for (int i = 0; i < logValues->lower_panel_log_value_id.length(); i++){
        QWidget *wg = ui->logBoxLayout->itemAt(i)->widget();
        QVBoxLayout* logVBoxLayout = wg->findChild<QVBoxLayout*>();
        if (logVBoxLayout){
            QLabel* log_label = wg->findChild<QLabel*>("log_label" + QString::number(i));
            if (log_label){

                for (int j = 0; j < logValues->log_value_id.count(); j++)
                {
                    if (logValues->log_value_id.at(j) == logValues->lower_panel_log_value_id.at(i) && logValues->log_value_protocol.at(j) == protocol)
                        index = j;
                }

                log_value = logValues->log_value.at(index).toDouble();
                unit = logValues->log_value_units.at(index).split(",").at(1);

                labelText = logValues->log_value.at(index);
                labelText.append(" <font size=1px color=grey>");
                labelText.append(unit);
                labelText.append("</font>");

                log_label->setAlignment(Qt::AlignRight);
                log_label->setText(labelText);
                int labelFontSize = size.width() / 90;
                QFont f("Arial",labelFontSize);
                log_label->setFont(f);
            }
        }
    }
    delay(1);
}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange)
    {
        if (this->isMaximized())
        {
            //qDebug() << "Maximize event";
            configValues->window_width = "maximized";
            configValues->window_height = "maximized";
        }
        else
        {
            QString window_width = QString::number(MainWindow::size().width());
            QString window_height = QString::number(MainWindow::size().height());

            configValues->window_width = window_width;
            configValues->window_height = window_height;
        }
        fileActions->save_config_file(configValues);
    }
    else if (event->type() == QEvent::Resize)
    {
        //qDebug() << "Screen resize event";

        if (isMaximized())
        {
            //qDebug() << "Window is maximized";
            configValues->window_width = "maximized";
            configValues->window_height = "maximized";
        }
        else
        {
            QString window_width = QString::number(MainWindow::size().width());
            QString window_height = QString::number(MainWindow::size().height());

            configValues->window_width = window_width;
            configValues->window_height = window_height;
        }

        fileActions->save_config_file(configValues);
    }


    return QWidget::event(event);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    //QScreen *screen = QGuiApplication::primaryScreen();
    //QRect size = screen->geometry();
    QWidget::resizeEvent(event);

    //QString window_width = QString::number(MainWindow::size().width());
    //QString window_height = QString::number(MainWindow::size().height());

    //qDebug() << "Screen resize event 2";

    QWidget* w = ui->mdiArea->findChild<QWidget*>("gaugeWindow");
    if (w){
        delete w;
    }
}

void MainWindow::set_status_bar_label(bool serialConnectionState, bool ecuConnectionState, QString romId)
{
    QString connectionStatusString;

    QString softwareVersionString = "FastECU";
    if (ecuConnectionState){
        connectionStatusString = "ECU connected";
        status_bar_connection_label->setStyleSheet("QLabel { background-color : green; color : white; color: white;}");
    }
    else if(serialConnectionState){
        connectionStatusString = "ECU not connected";
        status_bar_connection_label->setStyleSheet("QLabel { background-color : yellow; color : white; color: black;}");
    }
    else{
        connectionStatusString = "Serial port unavailable";
        status_bar_connection_label->setStyleSheet("QLabel { background-color : red; color : white; color: white;}");
    }
    QString romIdString = "ECU ID: " + romId;
    QString statusBarString = softwareVersionString + " | " + connectionStatusString + " | " + romIdString;
    status_bar_connection_label->setText(statusBarString);
}

void MainWindow::delay(int n)
{
    QTime dieTime = QTime::currentTime().addMSecs(n);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
}

void MainWindow::add_new_ecu_definition_file()
{
    QString filename;
    QObject* obj = sender();
    QListWidget* definition_files = obj->parent()->parent()->findChild<QListWidget*>("ecu_definition_files_list");

    QFileDialog openDialog;
    openDialog.setDefaultSuffix("xml");
    filename = QFileDialog::getOpenFileName(this, tr("Select definition file"), configValues->definition_files_directory, tr("ECU definition file (*.xml)"));

    if (filename.isEmpty())
    {
        QMessageBox::information(this, tr("ECU definition file"), "No file selected");
    }
    else
    {
        definition_files->addItem(filename);
        configValues->romraider_definition_files.append(filename);
        fileActions->save_config_file(configValues);
    }

}

void MainWindow::remove_ecu_definition_file()
{
    QObject* obj = sender();
    QListWidget* definition_files = obj->parent()->parent()->findChild<QListWidget*>("ecu_definition_files_list");
    QList<QModelIndex> index = definition_files->selectionModel()->selectedIndexes();

    int row = 0;
    for (int i = index.length() - 1; i >= 0; i--)
    {
        row = index.at(i).row();
        definition_files->model()->removeRow(row);
        configValues->romraider_definition_files.removeAt(row);
    }
    if (index.length() > 0)
        fileActions->save_config_file(configValues);
}

void MainWindow::add_new_logger_definition_file()
{
    QObject* obj = sender();

}

void MainWindow::remove_logger_definition_file()
{
    QObject* obj = sender();

}

QString MainWindow::parse_message_to_hex(QByteArray received)
{
    QString msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')));
    }

    return msg;
}

QString MainWindow::parse_ecuid(QByteArray received)
{
    QString msg;
    received.remove(0, 8);
    received.remove(5, received.length() - 5);

    for (int i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUpper());
    }

    return msg;
}
