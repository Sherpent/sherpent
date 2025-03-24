% Calcul du bilan énergétique du sherpent.
% Le but est de pouvoir dimensionner les composant et connaitre l'autonomie
Buck_out = 5;
LDO_out = 3.3;
LDO_eff = LDO_out/Buck_out;
ESP_current = 0.3;
ESP_voltage = 3.3;
ESP_power = ESP_current * ESP_voltage / LDO_eff;
Servo_current = 2.5;
Servo_power = Servo_current * Buck_out;
Buck_eff = 0.95
Power_buck_out = 2*Servo_power+ESP_power;
Power_buck_in = Power_buck_out/Buck_eff
vbat_min = 2.8;
MaxBatCurrent = Power_buck_in/vbat_min
Bat_capacity = 3 %Ah
BatteryLifeH = Bat_capacity*2/MaxBatCurrent %Life (heures)
BatteryLifeM = BatteryLifeH*60
