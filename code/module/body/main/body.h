//
// Created by Mathieu Durand on 2025-02-11.
//

#ifndef BODY_H
#define BODY_H

void ble_main();
void on_scan_stopped();
void on_connected();
void on_disconnected();
void message_callback(struct Message *message);

#endif // BODY_H