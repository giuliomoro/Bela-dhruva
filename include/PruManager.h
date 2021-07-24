/*
 * PruManager.h
 *
 * Support for interaction with PRU via
 * (rproc+mmap) and/or (uio+libprussdrv)
 *
 *	Created on: Jul 3, 2021
 *		Author: Dhruva Gole
 */

#include <string>

#if ENABLE_PRU_UIO == 1
#include <prussdrv.h>
#endif

#include <map>
#include "Mmap.h"

class PruManager
{
// expose parameters for the relevant paths
public:
	unsigned int pru_num, verbose;
	virtual int start(const char* path) = 0;
	virtual void stop() = 0;
	virtual void* getOwnMemory() = 0;
	virtual void* getSharedMemory() = 0;
};


#if ENABLE_PRU_RPROC == 1
class PruManagerRprocMmap : public PruManager
{
// use rproc for start/stop and mmap for memory sharing
public:
	PruManagerRprocMmap(unsigned int pruNum = 0, unsigned int v = 0);
	void stop();
	int start(const char* path);
	void* getOwnMemory();
	void* getSharedMemory();
private:
	std::map<unsigned int, unsigned int> pru_addr;	// prunum : pru address
	std::map<unsigned int, unsigned int> pruss_addr;	// prunum : pru sub-system address
	std::string basePath;
	std::string statePath;
	std::string firmwarePath;
	std::string firmware;
	std::string firmwareCopyCommand;
	unsigned int pru_num;
	unsigned int verbose;
	unsigned int pruss;
	unsigned int prucore;
	Mmap ownMemory;
	Mmap sharedMemory;
};
#endif	// ENABLE_PRU_RPROC

#if ENABLE_PRU_UIO == 1
class PruManagerUio : public PruManager
{
/* wrapper for libprussdrv for both start/stop and memory sharing
 * It has the libprussdrv calls currently present in the codebase
*/
public:
	PruManagerUio(unsigned int pruNum = 0, unsigned int v = 0);
	int start(const char* path);
	void stop();
	void* getOwnMemory();
	void* getSharedMemory();
private:
};

#endif	// ENABLE_PRU_UIO
