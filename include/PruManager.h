/* 
 * PruManager.h
 *
 * includes the classes required for introducing RProc functionality
 * but at the same time still support Mmap and libprussdrv
 *
 *  Created on: Jul 3, 2021
 *	  Author: Dhruva Gole
 */

#include <string>
// #ifdef flagUIO
#include <prussdrv.h>
// #endif
#include <map>

class PruManager{
// expose parameters for the relevant paths
public:
	std::map<unsigned int, unsigned int> pru_addr;   // prunum : pru address
	unsigned int pru_num, verbose;
	PruManager();
	// virtual void readstate() = 0;
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual void map_pru_mem(unsigned int pru_ram_id, char* address) = 0;
};

class PruManagerRprocMmap : public PruManager{
/* use rproc for start/stop and mmap for memory sharing
 */
public:
	PruManagerRprocMmap(unsigned int pruNum=0, unsigned int v=0);
    void readstate();
	void stop();
	void start();
	void map_pru_mem(unsigned int pru_ram_id, char* address);
private:
    int verbose = 0;
	std::string basePath;
	std::string statePath;
	std::string firmwarePath;
	std::string firmware;
	std::string firmwareCopyCommand;

};

class PruManagerUio : public PruManager{
/* wrapper for libprussdrv for both start/stop and memory sharing
 * It has the libprussdrv calls currently present in the codebase
*/
public:
	PruManagerUio(unsigned int pruNum=0, unsigned int v=0);
	void start();
	void stop();
	void map_pru_mem(unsigned int pru_ram_id, char* address);
};

class PruManagerAi : public PruManagerRprocMmap{
/* Might be needed for something AI specific?
 * Not completely sure yet.
 */
};
