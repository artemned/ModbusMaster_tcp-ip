#include "ModLogic.h"

using namespace std;
using namespace boost;



//
// 
// 
//


/**
 * Main Constructor of Modbus Object
 * @param host IP Address of Host
 * @param port Port for Connection
 * @return     A Modbus Object
 */
 /**
  * Overloading Modbus Constructor with Default Port at 502
  * @param host  IP Address of Host
  * @return      A Modbus Object
  */

modbus::modbus(boost::asio::io_service& serv,
	           std::string host, uint16_t port):_socket(serv)
{
	HOST = host;
	PORT = port;
	_slaveid = 1;
	_msg_id = 1;
	_connected = false;

}





/**
 * Destructor of Modbus Object
 */
 modbus:: ~modbus() {
	 modbus_close();
 }


/**
 * Set Slave ID
 * @param id  Id of Slave in Server to Set
 */
void modbus::modbus_set_slave_id(int id) {
	_slaveid = id;
}



/**
 * Build up Connection
 * @return   If A Connection Is Successfully Built

*/

bool modbus::modbus_connect()
{
	try
	{
		boost::asio::ip::tcp::endpoint endpoint_(boost::asio::ip::address::from_string(HOST), PORT);
		boost::system::error_code error;
		_socket.connect(endpoint_, error);
		if (error)
		{
			throw boost::system::system_error(error);
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Connect not succes" << std::endl;
		std::cerr << "error: " << e.what();
		return false;
	}

	return _connected=true;


}
 

 /**
  * Close the Connection
  */
void modbus::modbus_close() {
	_socket.close();
	std::cout << "Socket Closed" << std::endl;
}

bool modbus::get_connect() const
{
	return _connected;
}




/**
 * Modbus Request Builder
 * @param to_send   Message Buffer to Send
 * @param address   Reference Address
 * @param func      Functional Code
 */
void modbus::modbus_build_request(uint8_t* to_send, int address, int func) {
	to_send[0] = (uint8_t)_msg_id >> 8;//id transaction
	to_send[1] = (uint8_t)(_msg_id & 0x00FF);//id transaction
	to_send[2] = 0;
	to_send[3] = 0;
	to_send[4] = 0;
	to_send[6] = (uint8_t)_slaveid;
	to_send[7] = (uint8_t)func;
	to_send[8] = (uint8_t)(address >> 8);
	to_send[9] = (uint8_t)(address & 0x00FF);
}


/**
 * Write Request Builder and Sender
 * @param address   Reference Address
 * @param amount    Amount to Write
 * @param func      Functional Code
 * @param value     Value to Write
 */
void modbus::modbus_write(int address, int amount, int func, uint16_t* value) {


	if (func == WRITE_COIL || func == WRITE_REG) {
		uint8_t* to_send = new uint8_t[12];
		modbus_build_request(to_send, address, func);
		to_send[5] = 6;
		to_send[10] = (uint8_t)(value[0] >> 8);
		to_send[11] = (uint8_t)(value[0] & 0x00FF);
		modbus_send(to_send, 12);
		delete[]to_send;
	}
	else if (func == WRITE_REGS) {
		uint8_t* to_send = new uint8_t[13 + 2 * amount];
		modbus_build_request(to_send, address, func);
		to_send[5] = (uint8_t)(5 + 2 * amount);
		to_send[10] = (uint8_t)(amount >> 8);
		to_send[11] = (uint8_t)(amount & 0x00FF);
		to_send[12] = (uint8_t)(2 * amount);
		for (int i = 0; i < amount; i++) {
			to_send[13 + 2 * i] = (uint8_t)(value[i] >> 8);
			to_send[14 + 2 * i] = (uint8_t)(value[i] & 0x00FF);
		}
		modbus_send(to_send, 13 + 2 * amount);
		delete[]to_send;
	}
	else if (func == WRITE_COILS) {
		uint8_t* to_send = new uint8_t[14 + (amount - 1) / 8];
		modbus_build_request(to_send, address, func);
		to_send[5] = (uint8_t)(7 + (amount - 1) / 8);
		to_send[10] = (uint8_t)(amount >> 8);
		to_send[11] = (uint8_t)(amount >> 8);
		to_send[12] = (uint8_t)((amount + 7) / 8);
		for (int i = 0; i < amount; i++) {
			to_send[13 + (i - 1) / 8] += (uint8_t)(value[i] << (i % 8));
		}
		modbus_send(to_send, 14 + (amount - 1) / 8);
		delete[]to_send;
	}


}


/**
 * Read Requeset Builder and Sender
 * @param address   Reference Address
 * @param amount    Amount to Read
 * @param func      Functional Code
 */
void modbus::modbus_read(int address, int amount, int func) {
	uint8_t to_send[12];
	modbus_build_request(to_send, address, func);
	to_send[5] = 6;
	to_send[10] = (uint8_t)(amount >> 8);
	to_send[11] = (uint8_t)(amount & 0x00FF);
	modbus_send(to_send, 12);
}


/**
 * Read Holding Registers           MODBUS FUNCTION 0x03
 * @param address    Reference Address
 * @param amount     Amount of Registers to Read
 * @param buffer     Buffer to Store Data
 */
void modbus::modbus_read_holding_registers(int address, int amount, uint16_t* buffer) {
	if (_connected) {
		if (amount > 65535 || address > 65535) {
			throw modbus_amount_exception();
		}
		modbus_read(address, amount, READ_REGS);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, READ_REGS);
			for (int i = 0; i < amount; i++) {
				buffer[i] = ((uint16_t)to_rec[9 + 2 * i]) << 8;
				buffer[i] += (uint16_t)to_rec[10 + 2 * i];
			}
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete(&to_rec);
			delete(&k);
			throw e;
		}
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Read Input Registers             MODBUS FUNCTION 0x04
 * @param address     Reference Address
 * @param amount      Amount of Registers to Read
 * @param buffer      Buffer to Store Data
 */
void modbus::modbus_read_input_registers(int address, int amount, uint16_t* buffer) {
	if (_connected) {
		if (amount > 65535 || address > 65535) {
			throw modbus_amount_exception();
		}
		modbus_read(address, amount, READ_INPUT_REGS);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, READ_INPUT_REGS);
			for (int i = 0; i < amount; i++) {
				buffer[i] = ((uint16_t)to_rec[9 + 2 * i]) << 8;
				buffer[i] += (uint16_t)to_rec[10 + 2 * i];
			}
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete(&to_rec);
			delete(&k);
			throw e;
		}
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Read Coils           MODBUS FUNCTION 0x01
 * @param address     Reference Address
 * @param amount      Amount of Coils to Read
 * @param buffer      Buffers to Store Data
 */
void modbus::modbus_read_coils(int address, int amount, bool* buffer) {
	if (_connected) {
		if (amount > 2040 || address > 65535) {
			throw modbus_amount_exception();
		}
		modbus_read(address, amount, READ_COILS);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, READ_COILS);
			for (int i = 0; i < amount; i++) {
				buffer[i] = (bool)((to_rec[9 + i / 8] >> (i % 8)) & 1);
			}
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete(&to_rec);
			delete(&k);
			throw e;
		}
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Read Input Bits(Discrete Data)      MODBUS FUNCITON 0x02
 * @param address   Reference Address
 * @param amount    Amount of Bits to Read
 * @param buffer    Buffer to store Data
 */
void modbus::modbus_read_input_bits(int address, int amount, bool* buffer) {
	if (_connected) {
		if (amount > 2040 || address > 65535) {
			throw modbus_amount_exception();
		}
		modbus_read(address, amount, READ_INPUT_BITS);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, READ_INPUT_BITS);
			for (int i = 0; i < amount; i++) {
				buffer[i] = (bool)((to_rec[9 + i / 8] >> (i % 8)) & 1);
			}
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete(&to_rec);
			delete(&k);
			throw e;
		}
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Write Single Coils         MODBUS FUNCTION 0x05
 * @param address    Reference Address
 * @param to_write   Value to Write to Coil
 */
void modbus::modbus_write_coil(int address, bool to_write) {
	if (_connected) {
		if (address > 65535) {
			throw modbus_amount_exception();
		}
		int value = to_write * 0xFF00;
		modbus_write(address, 1, WRITE_COIL, (uint16_t*)& value);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, WRITE_COIL);
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete(&to_rec);
			delete(&k);
			throw e;
		}
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Write Single Register        FUCTION 0x06
 * @param address   Reference Address
 * @param value     Value to Write to Register
 */
void modbus::modbus_write_register(int address, uint16_t value) {
	if (_connected) {
		if (address > 65535) {
			throw modbus_amount_exception();
		}
		modbus_write(address, 1, WRITE_REG, &value);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, WRITE_COIL);
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete (&to_rec);
			delete (&k);
			throw e;
		}
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Write Multiple Coils        MODBUS FUNCTION 0x0F
 * @param address  Reference Address
 * @param amount   Amount of Coils to Write
 * @param value    Values to Write
 */
void modbus::modbus_write_coils(int address, int amount, bool* value) {
	if (_connected) {
		if (address > 65535 || amount > 65535) {
			throw modbus_amount_exception();
		}

		uint16_t* temp = new uint16_t[amount];
		for (int i = 0; i < 4; i++) {
			temp[i] = (uint16_t)value[i];
		}
		modbus_write(address, amount, WRITE_COILS, temp);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, WRITE_COILS);
			
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete (&to_rec);
			delete (&k);
			throw e;
		}
		delete[]temp;//dadsfsfsdfvsdgfdgfdgfgggggggggggg
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Write Multiple Registers    MODBUS FUNCION 0x10
 * @param address Reference Address
 * @param amount  Amount of Value to Write
 * @param value   Values to Write
 */
void modbus::modbus_write_registers(int address, int amount, uint16_t* value) {
	if (_connected) {
		if (address > 65535 || amount > 65535) {
			throw modbus_amount_exception();
		}
		modbus_write(address, amount, WRITE_REGS, value);
		uint8_t to_rec[MAX_MSG_LENGTH];
		size_t k = modbus_receive(to_rec);
		try {
			modbus_error_handle(to_rec, WRITE_REGS);
		}
		catch (std::exception& e) {
			cout << e.what() << endl;
			delete (&to_rec);
			delete (&k);
			throw e;
		}
	}
	else {
		throw modbus_connect_exception();
	}
}


/**
 * Data Sender
 * @param to_send Requeset to Send to Server
 * @param length  Length of the Request
 * @return        Size of the request
 */

 

 size_t modbus::modbus_send(uint8_t* to_send, int length) {
	 _msg_id++;
	 return _socket.send(boost::asio::buffer(to_send, length));
		 //send(_socket, (char*)to_send, (size_t)length, 0);
 }
 

 /**
  * Data Receiver
  * @param buffer Buffer to Store the Data
  * @return       Size of the Incoming Data
  */

  
  size_t modbus::modbus_receive(uint8_t* buffer) {
	  return _socket.receive(boost::asio::buffer(buffer, 1024));
		  //recv(_socket, (char*)buffer, 1024, 0);
  }
  

  /**
   * Error Code Handler
   * @param msg   Message Received from Server
   * @param func  Modbus Functional Code
   */
void modbus::modbus_error_handle(uint8_t* msg, int func) {
	if (msg[7] == func + 0x80) {
		switch (msg[8]) {
		case EX_ILLEGAL_FUNCTION:
			throw modbus_illegal_function_exception();
		case EX_ILLEGAL_ADDRESS:
			throw modbus_illegal_address_exception();
		case EX_ILLEGAL_VALUE:
			throw modbus_illegal_data_value_exception();
		case EX_SERVER_FAILURE:
			throw modbus_server_failure_exception();
		case EX_ACKNOWLEDGE:
			throw modbus_acknowledge_exception();
		case EX_SERVER_BUSY:
			throw modbus_server_busy_exception();
		case EX_GATEWAY_PROBLEMP:
		case EX_GATEWYA_PROBLEMF:
			throw modbus_gateway_exception();
		default:
			break;
		}
	}
}

