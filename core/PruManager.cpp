/*
 * PruManager.cpp
 * This code currently only works on BBAI
 *
 *	Created on: July 3, 2021
 *		Author: Dhruva Gole
 */

#include "../include/PruManager.h"
#include "MiscUtilities.h"
#include <iostream>

#if ENABLE_PRU_RPROC == 1
PruManagerRprocMmap::PruManagerRprocMmap(unsigned int pruNum, unsigned int v)
{
	/* constructor for initializing the necessary path variables
	 * based on the value of pru_num to choose:
	 * 0 for PRU1 core 0
	 * 1 for PRU1 core 1
	 * 2 for PRU2 core 0
	 * 3 for PRU2 core 1
	 */
	pru_num = pruNum;
	verbose = v;
	unsigned int pruss = pru_num / 2 + 1;
	unsigned int prucore = pru_num % 2;

	basePath = "/dev/remoteproc/pruss" + std::to_string(pruss) + "-core" + std::to_string(prucore) + "/";
	statePath = basePath + "state";
	firmwarePath = basePath + "firmware";
	firmware = "am57xx-pru" + std::to_string(pruss) + "_" + std::to_string(prucore) + "-fw";
# ifdef firmwareBelaRProc
	std::string firmwareBela = firmwareBelaRProc;	// Incoming string from Makefile
# endif	// firmwareBelaRProc
	firmwareCopyCommand = "sudo ln -s -f " + firmwareBela + " /lib/firmware/" + firmware; // Pass the name of custom .out file via Makefile using firmwareBelaRProc=<path to file>
	// 0 : pru1-core 0 in AI -> 4b234000
	// 1 : pru1-core 1 in AI -> 4b238000
	// 2 : pru2-core 0 in AI -> 4b2b4000
	// 3 : pru2-core 1 in AI -> 4b2b8000
	//
	// 1 : PRUSS address in AI for PRU 1
	// 2 : PRUSS address in AI for PRU 2
	//
	// 0 : pru-core 0 in BBB -> 4a334000
	// 1 : pru-core 1 in BBB -> 4a338000

# ifdef IS_AM572x	// base addresses for BBAI
	pru_addr.insert(std::pair<unsigned int, unsigned int>(0,0x4b234000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(1,0x4b238000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(2,0x4b2b4000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(3,0x4b2b8000));

	pruss_addr.insert(std::pair<unsigned int, unsigned int>(1,0x4b200000));
	pruss_addr.insert(std::pair<unsigned int, unsigned int>(2,0x4b280000));
# else	// base addresses for BBB
	pru_addr.insert(std::pair<unsigned int, unsigned int>(0,0x4a334000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(1,0x4a338000));
# endif	// IS_AM572x
}

void PruManagerRprocMmap::stop()
{	// performs echo stop > state
	if(verbose)
		std::cout << "Stopping the PRU" << std::to_string(pruss) + "_" + std::to_string(prucore) << "\n";
	IoUtils::writeTextFile(statePath, "stop");
}

int PruManagerRprocMmap::start(const char* path)
{
	stop();
	system(firmwareCopyCommand.c_str());	// copies fw to /lib/am57xx-fw
	if(verbose)
		std::cout << "Loading firmware into the PRU" << std::to_string(pruss) + "_" + std::to_string(prucore) << "\n";
	IoUtils::writeTextFile(firmwarePath,firmware);	// reload the new fw in PRU
	// performs echo start > state
	if(verbose)
		std::cout << "Starting the PRU" << std::to_string(pruss) + "_" + std::to_string(prucore) << "\n";
	IoUtils::writeTextFile(statePath, "start");
	return 0;	// TODO: If system returns any error then detect it and then return 1 instead
}

void* PruManagerRprocMmap::getOwnMemory()
{
	return ownMemory.map(pru_addr[pru_num], 0x2000);	// addr is full address of the start of the PRU's RAM
}

void* PruManagerRprocMmap::getSharedMemory()
{
	return sharedMemory.map(pruss_addr[pruss], 0x8000);
}
#endif	// ENABLE_PRU_RPROC

#if ENABLE_PRU_UIO == 1
#include "../include/PruBinary.h"

PruManagerUio::PruManagerUio(unsigned int pruNum, unsigned int v)
{
	// nothing to do
	pru_num = pruNum;
	verbose = v;
	prussdrv_init();
	if(prussdrv_open(PRU_EVTOUT_0)) {
		fprintf(stderr, "Failed to open PRU driver\n");
	}

}

int PruManagerUio::start(const char* path)
{
	if(path[0] == '\0') {
		unsigned int* pruCode;
		unsigned int pruCodeSize;
		pruCode = (unsigned int*)IrqPruCode::getBinary();
		pruCodeSize = IrqPruCode::getBinarySize();
		if(prussdrv_exec_code(pru_num, pruCode, pruCodeSize)) {
			fprintf(stderr, "Failed to execute PRU code\n");
			return 1;
		}
		else
			return 0;
	}
	else {
		if(prussdrv_exec_program(pru_num, path)) {
			return 1;
		}
		else
			return 0;
	}
}

void PruManagerUio::stop(){
	if(verbose)
		std::cout << "Stopping the PRU" << pru_num << "\n";
	prussdrv_pru_disable(pru_num);
}

void* PruManagerUio::getOwnMemory()
{
	void* pruDataRam;
	int ret = prussdrv_map_prumem (pru_num == 0 ? PRUSS0_PRU0_DATARAM : PRUSS0_PRU1_DATARAM, (void**)&pruDataRam);
	if(ret)
		return NULL;
	else
		return pruDataRam;
}


void* PruManagerUio::getSharedMemory()
{
	void* pruSharedRam;
	int ret = prussdrv_map_prumem (PRUSS0_SHARED_DATARAM, (void **)&pruSharedRam);
	if(ret)
		return NULL;
	else
		return pruSharedRam;
}
#endif	// for ENABLE_PRU_UIO
