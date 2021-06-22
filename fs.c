/*Nick Naslund & Christopher Hovsepian
3/14/21 Win21
Assignment 5
CSS430B Professor Dimpsey*/
// ============================================================================
// fs.c - user FileSytem API
// ============================================================================

#include "bfs.h"
#include "fs.h"
#include <stdbool.h>

// ============================================================================
// Close the file currently open on file descriptor 'fd'.
// ============================================================================
i32 fsClose(i32 fd) {
	i32 inum = bfsFdToInum(fd);
	bfsDerefOFT(inum);
	return 0;
}



// ============================================================================
// Create the file called 'fname'.  Overwrite, if it already exsists.
// On success, return its file descriptor.  On failure, EFNF
// ============================================================================
i32 fsCreate(str fname) {
	i32 inum = bfsCreateFile(fname);
	if (inum == EFNF) return EFNF;
	return bfsInumToFd(inum);
}



// ============================================================================
// Format the BFS disk by initializing the SuperBlock, Inodes, Directory and 
// Freelist.  On succes, return 0.  On failure, abort
// ============================================================================
i32 fsFormat() {
	FILE* fp = fopen(BFSDISK, "w+b");
	if (fp == NULL) FATAL(EDISKCREATE);

	i32 ret = bfsInitSuper(fp);               // initialize Super block
	if (ret != 0) { fclose(fp); FATAL(ret); }

	ret = bfsInitInodes(fp);                  // initialize Inodes block
	if (ret != 0) { fclose(fp); FATAL(ret); }

	ret = bfsInitDir(fp);                     // initialize Dir block
	if (ret != 0) { fclose(fp); FATAL(ret); }

	ret = bfsInitFreeList();                  // initialize Freelist
	if (ret != 0) { fclose(fp); FATAL(ret); }

	fclose(fp);
	return 0;
}


// ============================================================================
// Mount the BFS disk.  It must already exist
// ============================================================================
i32 fsMount() {
	FILE* fp = fopen(BFSDISK, "rb");
	if (fp == NULL) FATAL(ENODISK);           // BFSDISK not found
	fclose(fp);
	return 0;
}



// ============================================================================
// Open the existing file called 'fname'.  On success, return its file 
// descriptor.  On failure, return EFNF
// ============================================================================
i32 fsOpen(str fname) {
	i32 inum = bfsLookupFile(fname);        // lookup 'fname' in Directory
	if (inum == EFNF) return EFNF;
	return bfsInumToFd(inum);
}



// ============================================================================
// Read 'numb' bytes of data from the cursor in the file currently fsOpen'd on
// File Descriptor 'fd' into 'buf'.  On success, return actual number of bytes
// read (may be less than 'numb' if we hit EOF).  On failure, abort
// ============================================================================
i32 fsRead(i32 fd, i32 numb, void* buf) 
{
	i32 inum = bfsFdToInum(fd);
	i32 currentPointer = bfsTell(fd); //finds current cursor position
	i32 startFbn = currentPointer / BYTESPERBLOCK;
	i32 endFbn = (currentPointer + numb) / BYTESPERBLOCK;
	//checks to see if attempting to read past EOF
	i32 bytesRead = numb;
	i32 fileSize = bfsGetSize(inum);
	if (currentPointer + numb > fileSize)
	{
		bytesRead = fileSize - currentPointer;
		endFbn = (currentPointer + bytesRead) / BYTESPERBLOCK;
	}
	//allocates full length read buffer and individual DBN buffer
	i8 totalBlocks = endFbn - startFbn + 1;
	i8 readBuf[(totalBlocks)* BYTESPERBLOCK]; //buffer is 512 * number of fbn spanned i.e. 0-3 is 4
	i8 loopBuf[BYTESPERBLOCK];
	i32 bufferOffset = 0;
	//reads all spanned FBN into read buffer
	for (i32 i = startFbn; i <= endFbn; i++)
	{
		bfsRead(inum, i, loopBuf);
		memcpy((readBuf + bufferOffset), loopBuf, BYTESPERBLOCK);
		bufferOffset += BYTESPERBLOCK;
	}
	//copies portion of read buffer back into buf and returns bytes read (numb if not reading past EOF)
	memcpy(buf, (readBuf + (currentPointer % BYTESPERBLOCK)), bytesRead);
	fsSeek(fd, bytesRead, SEEK_CUR);
	return bytesRead;
}


// ============================================================================
// Move the cursor for the file currently open on File Descriptor 'fd' to the
// byte-offset 'offset'.  'whence' can be any of:
//
//  SEEK_SET : set cursor to 'offset'
//  SEEK_CUR : add 'offset' to the current cursor
//  SEEK_END : add 'offset' to the size of the file
//
// On success, return 0.  On failure, abort
// ============================================================================
i32 fsSeek(i32 fd, i32 offset, i32 whence) {

	if (offset < 0) FATAL(EBADCURS);

	i32 inum = bfsFdToInum(fd);
	i32 ofte = bfsFindOFTE(inum);

	switch (whence) {
	case SEEK_SET:
		g_oft[ofte].curs = offset;
		break;
	case SEEK_CUR:
		g_oft[ofte].curs += offset;
		break;
	case SEEK_END: {
		i32 end = fsSize(fd);
		g_oft[ofte].curs = end + offset;
		break;
	}
	default:
		FATAL(EBADWHENCE);
	}
	return 0;
}



// ============================================================================
// Return the cursor position for the file open on File Descriptor 'fd'
// ============================================================================
i32 fsTell(i32 fd) {
	return bfsTell(fd);
}



// ============================================================================
// Retrieve the current file size in bytes.  This depends on the highest offset
// written to the file, or the highest offset set with the fsSeek function.  On
// success, return the file size.  On failure, abort
// ============================================================================
i32 fsSize(i32 fd) {
	i32 inum = bfsFdToInum(fd);
	return bfsGetSize(inum);
}



// ============================================================================
// Write 'numb' bytes of data from 'buf' into the file currently fsOpen'd on
// filedescriptor 'fd'.  The write starts at the current file offset for the
// destination file.  On success, return 0.  On failure, abort
// ============================================================================
i32 fsWrite(i32 fd, i32 numb, void* buf) 
{
	i32 inum = bfsFdToInum(fd);
	i32 currentPointer = bfsTell(fd); //finds current cursor position
	i32 startFbn = currentPointer / BYTESPERBLOCK;
	i32 endFbn = (currentPointer + numb) / BYTESPERBLOCK;
	//checks to see if write is inside file or expansion is needed
	i32 fileSize = bfsGetSize(inum);
	i32 maxFileFbn = (fileSize - 1) / BYTESPERBLOCK;
	bool expandingWrite = false;
	if (endFbn > maxFileFbn)
	{
		bfsExtend(inum, endFbn);
		expandingWrite = true;
	}
	//allocates full length write buffer and individual DBN buffer
	i8 totalBlocks = endFbn - startFbn + 1;
	i8 writeBuf[(totalBlocks)* BYTESPERBLOCK]; //buffer is 512 * number of fbn spanned i.e. 0-3 is 4
	i8 blockBuf[BYTESPERBLOCK];
	//copies first fbn into writeBuf
	bfsRead(inum, startFbn, blockBuf);
	memcpy(writeBuf, blockBuf, BYTESPERBLOCK);
	//copies last fbn into writeBuf, if more than one fbn being written to
	if (totalBlocks > 1)
	{
		bfsRead(inum, endFbn, blockBuf);
		memcpy(writeBuf + (totalBlocks - 1) * BYTESPERBLOCK, blockBuf, BYTESPERBLOCK);
	}
	//copies buf into writeBuf with offset
	memcpy(writeBuf + currentPointer % BYTESPERBLOCK, buf, numb);
	//writes from writeBuf to each DBN one at a time
	i32 bufferOffset = 0;
	for (i32 i = startFbn; i <= endFbn; i++)
	{
		memcpy(blockBuf, writeBuf + bufferOffset, BYTESPERBLOCK);
		i32 thisDbn = bfsFbnToDbn(inum, i); //finds a dbn for a given inum and fbn
		bioWrite(thisDbn, blockBuf);
		bufferOffset += BYTESPERBLOCK;
	}
	//increases file's size variable if writing past EOF
	if (expandingWrite == true)
		bfsSetSize(inum, currentPointer + numb);
	fsSeek(fd, numb, SEEK_CUR);
	return 0;
}
