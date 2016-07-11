#include "pipelined.h"

namespace hirediscc {

Pipelined::Pipelined(ConnectionPtr conn)
	: connection_(conn){
}

void Pipelined::excute() {
	connection_->appendCommand(commandArgs_);
}

}

