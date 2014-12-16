

#ifndef _MEM_PROFILE_H_
#define _MEM_PROFILE_H_

void memprofile_start(const char* filepath, double time_resolution, 
		size_t size_resolution, const char* funcname);

void memprofile_insertnull();

void memprofile_end();

#endif // _MEM_PROFILE_H_
