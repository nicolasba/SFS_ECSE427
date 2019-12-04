
#ifndef SFS_API_H_
#define SFS_API_H_

void mksfs(int fresh);
int sfs_getnextfilename(char *fname);
int sfs_getfilesize(const char *path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_frseek(int fileID, int loc);
int sfs_fwseek(int fileID, int loc);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fwrite(int fileID, char *buf, int length);
int sfs_remove(char *file);

#endif /* SFS_API_H_ */
