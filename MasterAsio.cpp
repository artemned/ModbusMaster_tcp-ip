/*Example*/

#include "ModLogic.h"




int main()
{
	 boost::asio::io_service ser;
	 modbus client(ser,"127.0.0.1", 502);
	 client.modbus_set_slave_id(1);
	if (client.modbus_connect())
	{
		std::cout << "client true"<<endl;
	}
	uint16_t array[3];

	while (client.get_connect())
	{
		Sleep(1200);
		for (;;) {
			cout.unsetf(cout.dec);
			cout.setf(cout.hex);
			//bool read_coil=true;
			 //client.modbus_read_coils(0, 1, &read_coil);
			 uint16_t read_holding_regs[3];
			 client.modbus_read_holding_registers(0, 3,read_holding_regs);
			//client.modbus_read_input_registers(0, 3, read_holding_regs);
			//uint16_t write_regs[5] = { 123, 124, 787,1000,797 };
			 //client.modbus_write_registers(0, 5, write_regs);
			//bool write_cols[4] = { true, false, true, true };
			 // client.modbus_write_coils(0,3,write_cols);
			
			Sleep(1000);
			for (uint16_t i = 0; i < sizeof(read_holding_regs) / sizeof(read_holding_regs[0]); i++)
			{
				cout << read_holding_regs[i] << "\n";
			}
			cout << endl;
		}
	}
	return 0;
}