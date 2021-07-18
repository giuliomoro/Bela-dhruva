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

PruManager::PruManager()
{
	// Constructor might be of use later?
}

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
	firmwareCopyCommand = "sudo rm /lib/firmware/" + firmware + ";sudo ln -s /home/debian/Bela-dhruva/pru/blinkR30.pru1_0.out /lib/firmware/" + firmware; // **NOTE**: Change the name of .out file according afterwards here and below cases.
	// 0 : pruss1-core 0 in AI -> 4b234000
	// 1 : pruss1-core 1 in AI -> 4b238000
	// 2 : pruss2-core 0 in AI -> 4b2b4000
	// 3 : pruss2-core 1 in AI -> 4b2b8000
	//
	// 0 : pruss-core 0 in BBB -> 4a334000
	// 1 : pruss-core 1 in BBB -> 4a338000

#ifdef IS_AM572x	// base addresses for BBAI
	pru_addr.insert(std::pair<unsigned int, unsigned int>(0,0x4b234000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(1,0x4b238000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(2,0x4b2b4000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(3,0x4b2b8000));
#else	// base addresses for BBB
	pru_addr.insert(std::pair<unsigned int, unsigned int>(0,0x4a334000));
	pru_addr.insert(std::pair<unsigned int, unsigned int>(1,0x4a338000));
#endif
	long addr2 = 0x4b200000;
}

void PruManagerRprocMmap::readstate()
{	//Reads the current state of PRU
	state = IoUtils::readTextFile(statePath);
	if(verbose)
		std::cout << "PRU state is: " << state << "\n";
}

void PruManagerRprocMmap::stop()
{	//performs echo stop > state
	if(verbose)
		std::cout << "Stopping the PRU1_0 \n";
	IoUtils::writeTextFile(statePath, "stop");
}

int PruManagerRprocMmap::start()
{
	stop();
	system(firmwareCopyCommand.c_str());	// copies fw to /lib/am57xx-fw
	if(verbose)
		std::cout << "Loading firmware into the PRU1_0 \n";
	IoUtils::writeTextFile(firmwarePath,firmware);	// reload the new fw in PRU
	// performs echo start > state
	if(verbose)
		std::cout << "Starting the PRU1_0 \n";
	IoUtils::writeTextFile(statePath, "start");
	return 0;	// TODO: If system returns any error then detect it and then return 1 instead
}

void* PruManagerRprocMmap::getOwnMemory()
{
	return ownMemory.map(pru_addr[pru_num], 0x2000);	// addr is full address of the start of the PRU's RAM
}

void* PruManagerRprocMmap::getSharedMemory()
{
	return sharedMemory.map(addr2, 0x3000);	// addr2 is the address of the start of PRUSS Shared RAM
}
#endif	// for ENABLE_PRU_RPROC

#if ENABLE_PRU_UIO == 1
PruManagerUio::PruManagerUio(unsigned int pruNum, unsigned int v)
{
	// nothing to do
	pru_num = pruNum;
	verbose = v;
	start_status = 0;
}

void PruManagerUio::readstate()
{
	state = ((start_status == 1) ? "running" : "stopped");
	if(verbose)
		std::cout << "PRU state is: " << state << "\n";
}

int PruManagerUio::start()
{
	// prussdrv_init() I guess.
	// prussdrv_exec_program(pru_num, *filename);	// Maybe define filename in constructor
	prussdrv_init();
	if(prussdrv_open(PRU_EVTOUT_0)) {
		fprintf(stderr, "Failed to open PRU driver\n");
		return 1;
	}
	start_status = 1;	// keeps an internal flag that the PRU is running
	return 0;
}

void PruManagerUio::stop(){
	if(verbose)
		std::cout << "Stopping the PRU \n";
	start_status = 0;
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
