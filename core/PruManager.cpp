/*
 * PruManager.cpp
 *
 *	Created on: July 3, 2021
 *		Author: Dhruva Gole
 */

#include "PruManager.h"
#include "MiscUtilities.h"
#include <iostream>

PruManager::~PruManager()
{}

#if ENABLE_PRU_RPROC == 1
const std::vector<uint32_t> prussOwnRamOffsets = {0x0, 0x2000};
const uint32_t prussSharedRamOffset = 0x10000;

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
	pruss = pru_num / 2 + 1;
	prucore = pru_num % 2;

	basePath = "/dev/remoteproc/pruss" + std::to_string(pruss) + "-core" + std::to_string(prucore) + "/";
	statePath = basePath + "state";
	firmwarePath = basePath + "firmware";
	firmware = "am57xx-pru" + std::to_string(pruss) + "_" + std::to_string(prucore) + "-fw";
	// 0 : pru1-core 0 in AI -> 0x4b200000
	// 1 : pru1-core 1 in AI -> 0x4b202000
	// 2 : pru2-core 0 in AI -> 0x4b202000
	// 3 : pru2-core 1 in AI -> 0x4b282000
	//
	// 1 : PRUSS address in AI for PRU 1
	// 2 : PRUSS address in AI for PRU 2
	//
	// 0 : pru-core 0 in BBB -> 4a334000
	// 1 : pru-core 1 in BBB -> 4a338000

# ifdef IS_AM572x	// base addresses for BBAI
	pruRamAddr.insert(std::pair<unsigned int, unsigned int>(0, 0x4b200000));
	pruRamAddr.insert(std::pair<unsigned int, unsigned int>(1, 0x4b202000));
	pruRamAddr.insert(std::pair<unsigned int, unsigned int>(2, 0x4b280000));
	pruRamAddr.insert(std::pair<unsigned int, unsigned int>(3, 0x4b282000));

	sharedRamAddr.insert(std::pair<unsigned int, unsigned int>(1, 0x4b210000));
	sharedRamAddr.insert(std::pair<unsigned int, unsigned int>(2, 0x4b290000));
# else	// base addresses for BBB
	pruRamAddr.insert(std::pair<unsigned int, unsigned int>(0, 0x4a334000));
	pruRamAddr.insert(std::pair<unsigned int, unsigned int>(1, 0x4a338000));
# endif	// IS_AM572x
}

void PruManagerRprocMmap::stop()
{	// performs echo stop > state
	if(verbose)
		std::cout << "Stopping the PRU" << std::to_string(pruss) + "_" + std::to_string(prucore) << "\n";
	IoUtils::writeTextFile(statePath, "stop");
}

int PruManagerRprocMmap::start(bool useMcaspIrq)
{
# if (defined(firmwareBelaRProcNoMcaspIrq) && defined(firmwareBelaRProcMcaspIrq))
	std::string firmwareBela = useMcaspIrq ? firmwareBelaRProcMcaspIrq : firmwareBelaRProcNoMcaspIrq; // Incoming strings from Makefile
	return start(firmwareBela);
# else
# error No PRU firmware defined. Pass the name of firmware.out file using firmwareBelaRProcNoMcaspIrq=<path> and firmwareBelaRProcMcaspIrq=<path>
# endif	// firmwareBelaRProc
}

int PruManagerRprocMmap::start(const std::string& path)
{
	stop();
	std::string firmwareCopyCommand = "sudo ln -s -f " + path + " /lib/firmware/" + firmware;
	system(firmwareCopyCommand.c_str());	// copies fw to /lib/am57xx-fw
	if(verbose)
		std::cout << "Loading firmware into the PRU" << std::to_string(pruss) + "_" + std::to_string(prucore) << "\n";
	IoUtils::writeTextFile(firmwarePath, firmware);	// reload the new fw in PRU
	// performs echo start > state
	if(verbose)
		std::cout << "Starting the PRU" << std::to_string(pruss) + "_" + std::to_string(prucore) << "\n";
	IoUtils::writeTextFile(statePath, "start");
	return 0;	// TODO: If system returns any error then detect it and then return 1 instead
}

void* PruManagerRprocMmap::getOwnMemory()
{
	// return ownMemory.map(pruRamAddr[pru_num], 0x2000);	// addr is full address of the start of the PRU's RAM
	return ownMemory.map(prussAddresses[pruss] + prussOwnRamOffsets[prucore], 0x2000);
}

void* PruManagerRprocMmap::getSharedMemory()
{
	// return sharedMemory.map(sharedRamAddr[pruss], 0x8000);
	return sharedMemory.map(prussAddresses[pruss] + prussSharedRamOffset, 0x8000);
}
#endif	// ENABLE_PRU_RPROC

#if ENABLE_PRU_UIO == 1
#include "../include/PruBinary.h"

PruManagerUio::PruManagerUio(unsigned int pruNum, unsigned int v)
{
	pru_num = pruNum;
	verbose = v;
	prussdrv_init();
	if(prussdrv_open(PRU_EVTOUT_0)) {
		fprintf(stderr, "Failed to open PRU driver\n");
	}
}

int PruManagerUio::start(bool useMcaspIrq)
{
	unsigned int* pruCode;
	unsigned int pruCodeSize;
	switch((int)useMcaspIrq) // (int) is here to avoid stupid compiler warning
	{
		case false:
			pruCode = (unsigned int*)NonIrqPruCode::getBinary();
			pruCodeSize = NonIrqPruCode::getBinarySize();
			break;
		case true:
			pruCode = (unsigned int*)IrqPruCode::getBinary();
			pruCodeSize = IrqPruCode::getBinarySize();
			break;
	}
	if(prussdrv_exec_code(pru_num, pruCode, pruCodeSize)) {
		fprintf(stderr, "Failed to execute PRU code\n");
		return 1;
	}
	else
		return 0;
}

int PruManagerUio::start(const std::string& path)
{
	if(prussdrv_exec_program(pru_num, path.c_str())) {
		return 1;
	}
	return 0;
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
#endif	// ENABLE_PRU_UIO
