/*
 * PruManager.cpp
 * This code currently only works on BBAI
 *
 *  Created on: July 3, 2021
 *	  Author: Dhruva Gole
 */

#include "../include/PruManager.h"
#include "MiscUtilities.h"
#include <iostream>

#ifdef IS_AM572x1
#define AM572x_Flag 1
#else
#define AM572x_Flag 0
#endif

PruManager::PruManager(){
	// ToDo: add base addresses to be used by Mmap
	// 0 : pruss-core 0 in BBB -> 4a334000
	// 1 : pruss-core 1 in BBB -> 4a338000
	// 0 : pruss1-core 0 in AI -> 4b234000
	// 1 : pruss1-core 1 in AI -> 4b238000
	// 2 : pruss2-core 0 in AI -> 4b2b4000
	// 3 : pruss2-core 1 in AI -> 4b2b8000
	if(AM572x_Flag){
		pru_addr.insert(std::pair<unsigned int, unsigned int>(0,0x4b234000));
		pru_addr.insert(std::pair<unsigned int, unsigned int>(1,0x4b238000));
		pru_addr.insert(std::pair<unsigned int, unsigned int>(2,0x4b2b4000));
		pru_addr.insert(std::pair<unsigned int, unsigned int>(3,0x4b2b8000));
	}
	else{
		pru_addr.insert(std::pair<unsigned int, unsigned int>(0,0x4a334000));
		pru_addr.insert(std::pair<unsigned int, unsigned int>(1,0x4a338000));
	}
}

PruManagerRprocMmap::PruManagerRprocMmap(unsigned int pruNum, unsigned int v){
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
}

void PruManagerRprocMmap::readstate(){	//Reads the current state of PRU
	std::string state = IoUtils::readTextFile(statePath);
	if(verbose)
		std::cout << "PRU state is: " << state << "\n";
}

void PruManagerRprocMmap::stop(){	//performs echo stop > state
	if(verbose)
		std::cout << "Stopping the PRU1_0 \n";
	//mode = TRUNCATE; by default
	IoUtils::writeTextFile(statePath, "stop");
}

void PruManagerRprocMmap::start(){	// performs echo start > state
	if(verbose)
		std::cout << "Starting the PRU1_0 \n";
	//mode = TRUNCATE; by default
	IoUtils::writeTextFile(statePath, "start");
	system(firmwareCopyCommand.c_str());    // copies fw to /lib/am57xx-fw	
    if(verbose)
		std::cout << "Loading firmware into the PRU1_0 \n";
	//mode = TRUNCATE; by default
	IoUtils::writeTextFile(firmwarePath,firmware);  // reload the new fw in PRU

}

void PruManagerRprocMmap::map_pru_mem(unsigned int pru_ram_id, void **address){
	//do nothing for now.
}

PruManagerUio::PruManagerUio(unsigned int pruNum, unsigned int v){
	// nothing to do
	pru_num = pruNum;
	verbose = v;
}

void PruManagerUio::start(){
	// prussdrv_init() I guess.
	// prussdrv_exec_program(pru_num, *filename);   // Maybe define filename in constructor
}

void PruManagerUio::stop(){
	// prussdrv_stop() equivalent
	if(verbose)
		std::cout << "Stopping the PRU \n";
	prussdrv_pru_disable(pru_num);
}

void PruManagerUio::map_pru_mem(unsigned int pru_ram_id, void **address){
	prussdrv_map_prumem (pru_ram_id, (void **)&address);
}
