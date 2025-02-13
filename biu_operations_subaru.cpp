#include "biu_operations_subaru.h"
#include <ui_biu_operations_subaru.h>

BiuOperationsSubaru::BiuOperationsSubaru(SerialPortActions *serial, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::BiuOperationsSubaruWindow)
{
    ui->setupUi(this);
    this->setParent(parent);

    ui->progressbar->hide();

    this->serial = serial;

    biuOpsSubaruSwitchesIo = nullptr;
    biuOpsSubaruSwitchesLighting = nullptr;
    biuOpsSubaruSwitchesOptions = nullptr;
    biuOpsSubaruDataDtcs = nullptr;
    biuOpsSubaruDataBiu = nullptr;
    biuOpsSubaruDataCan = nullptr;
    biuOpsSubaruDataTt = nullptr;
    biuOpsSubaruDataVdcabs = nullptr;
    biuOpsSubaruDataDest = nullptr;
    biuOpsSubaruDataFactory = nullptr;

    counter = 0;
    switch_result = new QStringList();
    data_result = new QStringList();
    keep_alive_timer = new QTimer(this);
    keep_alive_timer->setInterval(1000);
    current_command = NO_COMMAND;
    connection_state = NOT_CONNECTED;
    output.clear();

    for (int i = 0; i < biu_messages.length(); i+=2)
    {
        ui->msg_combo_box->addItem(biu_messages.at(i));
    }

    connect(ui->send_msg, SIGNAL(clicked(bool)), this, SLOT(prepare_biu_msg()));
    connect(keep_alive_timer, SIGNAL(timeout()), this, SLOT(keep_alive()));

}

BiuOperationsSubaru::~BiuOperationsSubaru()
{
    //delete ui;
    keep_alive_timer->stop();
}

BiuOpsSubaruSwitches* BiuOperationsSubaru::update_biu_ops_subaru_switches_window(BiuOpsSubaruSwitches *biuOpsSubaruSwitches)
{
    if (biuOpsSubaruSwitches == nullptr)
    {
        biuOpsSubaruSwitches = new BiuOpsSubaruSwitches(switch_result);
        biuOpsSubaruSwitches->show();
    }
    else
    {
        if (!biuOpsSubaruSwitches->isVisible()) biuOpsSubaruSwitches->show();
        biuOpsSubaruSwitches->update_switch_results(switch_result);
    }

    return biuOpsSubaruSwitches;

}

BiuOpsSubaruData* BiuOperationsSubaru::update_biu_ops_subaru_data_window(BiuOpsSubaruData *biuOpsSubaruData)
{
    if (biuOpsSubaruData == nullptr)
    {
        biuOpsSubaruData = new BiuOpsSubaruData(data_result);
        biuOpsSubaruData->show();
    }
    else
    {
        if (!biuOpsSubaruData->isVisible()) biuOpsSubaruData->show();
        biuOpsSubaruData->update_data_results(data_result);
    }

    return biuOpsSubaruData;

}

void BiuOperationsSubaru::close_results_windows()
{
    if (biuOpsSubaruSwitchesIo != nullptr) biuOpsSubaruSwitchesIo->hide();
    if (biuOpsSubaruSwitchesLighting != nullptr) biuOpsSubaruSwitchesLighting->hide();
    if (biuOpsSubaruSwitchesOptions != nullptr) biuOpsSubaruSwitchesOptions->hide();
    if (biuOpsSubaruDataDtcs != nullptr) biuOpsSubaruDataDtcs->hide();
    if (biuOpsSubaruDataBiu != nullptr) biuOpsSubaruDataBiu->hide();
    if (biuOpsSubaruDataCan != nullptr) biuOpsSubaruDataCan->hide();
    if (biuOpsSubaruDataTt != nullptr) biuOpsSubaruDataTt->hide();
    if (biuOpsSubaruDataVdcabs != nullptr) biuOpsSubaruDataVdcabs->hide();
    if (biuOpsSubaruDataDest != nullptr) biuOpsSubaruDataDest->hide();
    if (biuOpsSubaruDataFactory != nullptr) biuOpsSubaruDataFactory->hide();
}

void BiuOperationsSubaru::closeEvent(QCloseEvent *event)
{
    qDebug() << "Closing BIU log window";
    keep_alive_timer->stop();
    if (biuOpsSubaruSwitchesIo != nullptr) biuOpsSubaruSwitchesIo->close();
    if (biuOpsSubaruSwitchesLighting != nullptr) biuOpsSubaruSwitchesLighting->close();
    if (biuOpsSubaruSwitchesOptions != nullptr) biuOpsSubaruSwitchesOptions->close();
    if (biuOpsSubaruDataDtcs != nullptr) biuOpsSubaruDataDtcs->close();
    if (biuOpsSubaruDataBiu != nullptr) biuOpsSubaruDataBiu->close();
    if (biuOpsSubaruDataCan != nullptr) biuOpsSubaruDataCan->close();
    if (biuOpsSubaruDataTt != nullptr) biuOpsSubaruDataTt->close();
    if (biuOpsSubaruDataVdcabs != nullptr) biuOpsSubaruDataVdcabs->close();
    if (biuOpsSubaruDataDest != nullptr) biuOpsSubaruDataDest->close();
    if (biuOpsSubaruDataFactory != nullptr) biuOpsSubaruDataFactory->close();

}

void BiuOperationsSubaru::keep_alive()
{

    if (current_command == TESTER_PRESENT)
    {
        output.clear();
        output.append((uint8_t)0x81);
        output.append((uint8_t)0x40);
        output.append((uint8_t)0xf0);
        output.append((uint8_t)0x3E);
        output.append((uint8_t)0xEF);
    }

    send_biu_msg();
}

void BiuOperationsSubaru::prepare_biu_msg()
{

    QString selected_item_text = ui->msg_combo_box->currentText();
    QStringList selected_item_msg;

    bool ok = false;
    uint8_t chk_sum;

    if (selected_item_text != "Custom")
    {
        for (int i = 0; i < biu_messages.length(); i+=2)
        {
            if (selected_item_text == biu_messages.at(i))
                selected_item_msg = biu_messages.at(i + 1).split(",");
        }
    }
    else
    {
        if (ui->msg_line_edit->text() != "")
            selected_item_msg = ui->msg_line_edit->text().split(",");
    }

    output.clear();
    output.append((uint8_t)0x80);
    output.append((uint8_t)0x40);
    output.append((uint8_t)0xf0);
    for (int i = 0; i < selected_item_msg.length(); i++)
    {
        output.append(selected_item_msg.at(i).toUInt(&ok, 16));
    }
    output[0] = output[0] | (output.length() - 3);
    chk_sum = calculate_checksum(output, false);
    output.append((uint8_t) chk_sum);

    current_command = output[3];
    if (current_command == INFO_REQUEST) current_command = output[4];

    send_biu_msg();
}

void BiuOperationsSubaru::send_biu_msg()
{

    keep_alive_timer->stop();

    QByteArray received;

    if (connection_state == NOT_CONNECTED && current_command != CONNECT)
    {
        send_log_window_message("Not connected, can't send command", true, true);
        return;
    }

    send_log_window_message("Send msg: " + parse_message_to_hex(output), true, true);

    if (connection_state == NOT_CONNECTED && current_command == CONNECT)
    {
        serial->fast_init(output);
        //delay(100);

    }
    else
        received = serial->write_serial_data_echo_check(output);

    received = serial->read_serial_data(100, 100);

    send_log_window_message("Received msg: " + parse_message_to_hex(received), true, true);
    parse_biu_message(received);

    if (connection_state == CONNECTED) keep_alive_timer->start();
}

uint8_t BiuOperationsSubaru::calculate_checksum(QByteArray output, bool exclude_last_byte)
{
    uint8_t checksum = 0;
    int len = output.length();
    if (exclude_last_byte) len--;

    for (uint16_t i = 0; i < len; i++) checksum += (uint8_t)output.at(i);

    return checksum;
}

void BiuOperationsSubaru::parse_biu_message(QByteArray message)
{
    uint8_t chk_sum;

    chk_sum = calculate_checksum(message, true);
    if (((uint8_t)message.at(0) & 0x80) != 0x80 || (uint8_t)message.at(1) != 0xf0 || (uint8_t)message.at(2) != 0x40)
    {
        send_log_window_message("Invalid message received: invalid header", true, true);
        return;
    }

    if (((uint8_t)message.at(0) & 0x7F) != (uint8_t)message.length() - 4)
    {
        send_log_window_message("Invalid message received: invalid length", true, true);
        return;
    }

    if (chk_sum != (uint8_t)message.at(message.length() - 1))
    {
        send_log_window_message("Invalid message received: invalid checksum", true, true);
        return;
    }

    if ((uint8_t)message.at(3) == (CONNECT + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x81 cksm
         * rsp:   fm+l dest src  rply 0xK1 0xK2 cksm
         */

        send_log_window_message("Connection to BIU successful", true, true);
        connection_state = CONNECTED;
        current_command = TESTER_PRESENT;
    }
    else if ((uint8_t)message.at(3) == (DISCONNECT + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x82 cksm
         * rsp:   fm+l dest src  rply cksm
         */

        send_log_window_message("Disconnection from BIU successful", true, true);
        connection_state = NOT_CONNECTED;
        current_command = NO_COMMAND;
        close_results_windows();
    }
    else if ((uint8_t)message.at(3) == (DTC_READ + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x18 cksm
         * rsp:   fm+l dest src  rply 0xD1 0xD2 0xD3 cksm
         */

        //QStringList dtc_result;
        QString dtc_code;
        QString byte;

        current_command = TESTER_PRESENT;

        int index = 5;

        data_result->clear();

        if (message.length() >= (index + 4))
        {
            send_log_window_message("BIU DTC list:", true, true);

            while (index < (message.length() - 1))
            {
                byte = QString("%1").arg(message.at(index) & 0x0f,2,16,QLatin1Char('0'));
                dtc_code = "B" + byte;
                index++;
                byte = QString("%1").arg(message.at(index) & 0xff,2,16,QLatin1Char('0'));
                dtc_code.append(byte);
                index++;
                for (int i = 0; i < biu_dtc_list.length(); i+=2)
                {
                    if (dtc_code == biu_dtc_list.at(i))
                        dtc_code.append(" - " + biu_dtc_list.at(i + 1));
                }
                if (message.at(index) == 0)
                    dtc_code.append(" (pending)");
                else
                    dtc_code.append(" (stored)");
                index++;

                if (dtc_code != "")
                {
                    //send_log_window_message(dtc_code, true, true);
                    data_result->append(dtc_code);
                }
            }

        }
        else
        {
            data_result->append("No BIU DTC found");
            send_log_window_message("No BIU DTC found", true, true);
        }

        biuOpsSubaruDataDtcs = update_biu_ops_subaru_data_window(biuOpsSubaruDataDtcs);

    }
    else if ((uint8_t)message.at(3) == (DTC_CLEAR + 0x40))
    {
        /*
         * index: 0    1    2    3    4    5    6
         * cmd:   fm+l dest src  0x14 0x40 0x00 cksm
         * rsp:   fm+l dest src  rply 0x40 0x00 cksm
         */

        current_command = TESTER_PRESENT;

        send_log_window_message("BIU DTCs successfully cleared", true, true);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == IN_OUT_SWITCHES)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x50 cksm
         * rsp:   fm+l dest src  rply 0x50 0xB1 0xB2 0xBn cksm
         */

        int index = 5;
        int i;

        switch_result->clear();

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                int bit_mask = 1;
                for (int bit_counter = 0; bit_counter < 8; bit_counter++)
                {
                    i = ((index - 5) * 8) + bit_counter;
                    switch_result->append(biu_switch_names.at(i));
                    if ((uint8_t)message.at(index) & bit_mask) switch_result->append("ON");
                    else switch_result->append("OFF");
                    //send_log_window_message(switch_result->at(2 * i) + switch_result->at(2 * i + 1), true, true);
                    bit_mask = bit_mask << 1;
                }
            }
        }

        biuOpsSubaruSwitchesIo = update_biu_ops_subaru_switches_window(biuOpsSubaruSwitchesIo);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == LIGHTING_SWITCHES)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x51 cksm
         * rsp:   fm+l dest src  rply 0x51 0xB1 0xB2 0xBn cksm
         */

        int index = 5;
        int i;

        switch_result->clear();

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                int bit_mask = 1;
                for (int bit_counter = 0; bit_counter < 8; bit_counter++)
                {
                    i = ((index - 5) * 8) + bit_counter;
                    switch_result->append(biu_lightsw_names.at(i));
                    if ((uint8_t)message.at(index) & bit_mask) switch_result->append("ON");
                    else switch_result->append("OFF");
                    //send_log_window_message(switch_result, true, true);
                    bit_mask = bit_mask << 1;
                }
            }

        }

        biuOpsSubaruSwitchesLighting = update_biu_ops_subaru_switches_window(biuOpsSubaruSwitchesLighting);
    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == BIU_DATA)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x40 cksm
         * rsp:   fm+l dest src  rply 0x40 0xB1 0xB2 0xBn cksm
         */

        float calc_result;
        QString biu_data_result;
        int index = 5;

        data_result->clear();

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                biu_data_result = biu_data_names.at((index - 5) * 2);
                calc_result = ((uint8_t)message.at(index) * biu_data_factors[(index - 5) * 2]) + biu_data_factors[(index - 5) * 2 + 1];
                biu_data_result.append(QString("%1 ").arg(calc_result));
                biu_data_result.append(biu_data_names.at((index - 5) * 2 + 1));
                data_result->append(biu_data_result);
                //send_log_window_message(data_result, true, true);
            }

        }

        biuOpsSubaruDataBiu = update_biu_ops_subaru_data_window(biuOpsSubaruDataBiu);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == CAN_DATA)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x41 cksm
         * rsp:   fm+l dest src  rply 0x41 0xB1 0xB2 0xBn cksm
         */

        QString can_data_result;
        float calc_result;

        int item = 0;

        data_result->clear();

        if (message.length() >= 7)
        {

            // front wheel speed
            can_data_result = can_data_names.at(item * 2);
            calc_result = ((uint8_t)message.at(6) << 8) | (uint8_t)message.at(5);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // VDC/ABS latest f-code
            item++;
            can_data_result = can_data_names.at(item * 2);
            can_data_result.append(QString("%1 ").arg((uint8_t)message.at(8),2,16,QLatin1Char('0')));
            can_data_result.append(QString("%1 ").arg((uint8_t)message.at(7),2,16,QLatin1Char('0')));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // Blower fan steps
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(9);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // Fuel level resistance
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = ((uint8_t)message.at(11) << 8) | (uint8_t)message.at(10);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // Fuel consumption
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = ((uint8_t)message.at(13) << 8) | (uint8_t)message.at(12);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // engine coolant temp
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(14);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // g-force
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(15);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // sport shift
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(16);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

            // shift position
            item++;
            can_data_result = can_data_names.at(item * 2);
            calc_result = (uint8_t)message.at(17);
            calc_result = (calc_result * can_data_factors[item * 2]) + can_data_factors[item * 2 + 1];
            can_data_result.append(QString("%1 ").arg(calc_result));
            can_data_result.append(can_data_names.at(item * 2 + 1));
            data_result->append(can_data_result);
            //send_log_window_message(can_data_result, true, true);

        }

        biuOpsSubaruDataCan = update_biu_ops_subaru_data_window(biuOpsSubaruDataCan);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == BIU_CUSTOM_TIME_TEMP)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x52 cksm
         * rsp:   fm+l dest src  rply 0x52 0xB1 0xB2 0xBn cksm
         */

        current_command = TESTER_PRESENT;
        QString biu_tt_result;
        float calc_result;

        data_result->clear();

        if (message.length() >= 7)
        {

            // room lamp off delay time
            biu_tt_result = biu_tt_names.at(0);
            calc_result = (uint8_t)message.at(5) & 0x03;
            if (calc_result == 0) biu_tt_result.append("OFF");
            else if (calc_result == 0x01) biu_tt_result.append("Short");
            else if (calc_result == 0x02) biu_tt_result.append("Normal");
            else if (calc_result == 0x03) biu_tt_result.append("Long");
            biu_tt_result.append(biu_tt_names.at(1));
            data_result->append(biu_tt_result);
            //send_log_window_message(biu_tt_result, true, true);

            // auto-lock time
            biu_tt_result = biu_tt_names.at(2);
            calc_result = (((uint8_t)message.at(6) & 0x07) + 2) * 10;
            biu_tt_result.append(QString("%1 ").arg(calc_result));
            biu_tt_result.append(biu_tt_names.at(3));
            data_result->append(biu_tt_result);
            //send_log_window_message(biu_tt_result, true, true);

            // outside temp offset
            if (message.length() == 9)
            {
                biu_tt_result = biu_tt_names.at(4);
                calc_result = (((uint8_t)message.at(7) & 0x0F) - 4) * 0.5;
                biu_tt_result.append(QString("%1 ").arg(calc_result));
                biu_tt_result.append(biu_tt_names.at(5));
                data_result->append(biu_tt_result);
                //send_log_window_message(biu_tt_result, true, true);
            }
        }

        biuOpsSubaruDataTt = update_biu_ops_subaru_data_window(biuOpsSubaruDataTt);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == CAR_OPTIONS)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x53 cksm
         * rsp:   fm+l dest src  rply 0x53 0xB1 0xB2 0xBn cksm
         */

        //QString biu_options_result;

        current_command = TESTER_PRESENT;
        switch_result->clear();

        int index = 5;
        int i;

        if (message.length() >= (index + 2))
        {
            for (index = 5; index < (message.length() - 1); index++)
            {
                int bit_mask = 1;
                for (int bit_counter = 0; bit_counter < 8; bit_counter++)
                {
                    i = ((index - 5) * 8) + bit_counter;
                    switch_result->append(biu_options_names.at(i * 3));
                    if ((uint8_t)message.at(index) & bit_mask) switch_result->append(biu_options_names.at(i * 3 + 1));
                    else switch_result->append(biu_options_names.at(i * 3 + 2));
                    //send_log_window_message(switch_result->at(2 * i) + switch_result->at(2 * i + 1), true, true);
                    bit_mask = bit_mask << 1;
                }
            }

        }

        biuOpsSubaruSwitchesOptions = update_biu_ops_subaru_switches_window(biuOpsSubaruSwitchesOptions);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == VDC_ABS_CONDITION)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x60 cksm
         * rsp:   fm+l dest src  rply 0x60 0xB1 cksm
         */

        current_command = TESTER_PRESENT;
        data_result->clear();

        int condition = (uint8_t)message.at(5) & 0x07;
        data_result->append("VDC/ABS Condition: " + QString::number(condition));
        //send_log_window_message(data_result, true, true);

        biuOpsSubaruDataVdcabs = update_biu_ops_subaru_data_window(biuOpsSubaruDataVdcabs);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == DEST_TOUCH_STATUS)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x61 cksm
         * rsp:   fm+l dest src  rply 0x61 0xB1 0xB2 cksm
         */

        int condition;
        current_command = TESTER_PRESENT;
        data_result->clear();

        condition = (uint8_t)message.at(5) & 0x0F;
        data_result->append("Destination:    " + QString::number(condition));
        condition = (uint8_t)message.at(6) & 0x3F;
        data_result->append("Touchscreen SW: " + QString::number(condition));
        //send_log_window_message(data_result, true, true);

        biuOpsSubaruDataDest = update_biu_ops_subaru_data_window(biuOpsSubaruDataDest);

    }
    else if ((uint8_t)message.at(3) == (INFO_REQUEST + 0x40) && (uint8_t)message.at(4) == FACTORY_STATUS)
    {
        /*
         * index: 0    1    2    3    4    5    6    7
         * cmd:   fm+l dest src  0x21 0x54 cksm
         * rsp:   fm+l dest src  rply 0x54 0xB1 cksm
         */

        QString setting;
        current_command = TESTER_PRESENT;
        data_result->clear();

        if ((uint8_t)message.at(5) & 0x01) setting = "Factory";
        else setting = "Market";
        data_result->append("Factory Initial Setting: " + setting);
        //send_log_window_message(data_result, true, true);

        biuOpsSubaruDataFactory = update_biu_ops_subaru_data_window(biuOpsSubaruDataFactory);

    }
    else if ((uint8_t)message.at(3) == 0x7F)
    {
        send_log_window_message("Error response received from BIU", true, true);
    }
}

QString BiuOperationsSubaru::parse_message_to_hex(QByteArray received)
{
    QByteArray msg;

    for (unsigned long i = 0; i < received.length(); i++)
    {
        msg.append(QString("%1 ").arg((uint8_t)received.at(i),2,16,QLatin1Char('0')).toUtf8());
    }

    return msg;
}

int BiuOperationsSubaru::send_log_window_message(QString message, bool timestamp, bool linefeed)
{
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = dateTime.toString("[yyyy-MM-dd hh':'mm':'ss'.'zzz']  ");

    if (timestamp)
        message = dateTimeString + message;
    if (linefeed)
        message = message + "\n";

    QTextEdit* textedit = this->findChild<QTextEdit*>("text_edit");
    if (textedit)
    {
        ui->text_edit->insertPlainText(message);
        ui->text_edit->ensureCursorVisible();

        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

        return STATUS_SUCCESS;
    }

    return STATUS_ERROR;
}

void BiuOperationsSubaru::delay(int timeout)
{
    QTime dieTime = QTime::currentTime().addMSecs(timeout);
    while (QTime::currentTime() < dieTime)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}
