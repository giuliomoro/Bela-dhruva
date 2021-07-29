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
#include <vector>
#include "Mmap.h"

class PruManager
{
// expose parameters for the relevant paths
public:
	unsigned int pru_num, verbose;
	virtual int start(bool useMcaspIrq) = 0;
	virtual int start(const std::string& path) = 0;
	virtual void stop() = 0;
	virtual void* getOwnMemory() = 0;
	virtual void* getSharedMemory() = 0;
	virtual ~PruManager() = 0;
};

#if ENABLE_PRU_RPROC == 1
class PruManagerRprocMmap : public PruManager
{
// use rproc for start/stop and mmap for memory sharing
public:
	PruManagerRprocMmap(unsigned int pruNum = 0, unsigned int v = 0);
	void stop();
	int start(bool useMcaspIrq);
	int start(const std::string& path);
	void* getOwnMemory();
	void* getSharedMemory();
private:
	std::vector<uint32_t> prussAddresses;
	std::map<unsigned int, unsigned int> pruRamAddr;	// prunum : pru address
	std::map<unsigned int, unsigned int> sharedRamAddr;	// pruss : pru sub-system address
	std::string basePath;
	std::string statePath;
	std::string firmwarePath;
	std::string firmware;
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
	int start(bool useMcaspIrq);
	int start(const std::string& path);
	void stop();
	void* getOwnMemory();
	void* getSharedMemory();
private:
};

#endif	// ENABLE_PRU_UIO
