#include <iostream>
#include <stdio.h>

class FileReader {
	public:
		explicit FileReader(char* file_name)
			: pos(0), end(0) {
			fp = fopen(file_name, "r");
			if (fp == NULL) {
				std::cout<<"open file " << file_name << " failed" <<std::endl;
			}
		}

		int getBytes(uint8_t** out_buf, int len) {
			if (end - pos < len) {
				//read data
				if (readData() < 0) {
					return -1;
				}
				if (end - pos < len) {
					std::cout<<"data not enough"<<std::endl;
					return -1;
				}
			}
			*out_buf = (uint8_t*)(buf+pos);
			return 0;
		}

		int advance(int len) {
			if (pos + len > end) {
				std::cout<<"pos: " <<pos<<" len: " <<len<<" end: "<<end<<std::endl;
				return -1;
			}
			pos += len;
			return 0;
		}

		int getCurPos() {
			long n = ftell(fp);
			return n - (end - pos);
		}

	private:
		int readData() {
			if (fp == NULL) {
				std::cout<<"fp is null"<<std::endl;
				return -1;
			}
			if (pos < end) {
				memmove(buf, buf+pos, end - pos);
			}
			end = end - pos;
			pos = 0;
			int ret = fread((void*)(buf+end), 1, sizeof(buf) - end, fp);
			if (ret > 0) {
				end += ret;
			} else {
				std::cout<<"reach file end"<<std::endl;
				return -1;
			}

			return 0;
		}

		FILE* fp;
		uint8_t buf[4096];
		int end;
		int pos;

};

int main(int argc, char** argv) {
	if (argc < 3) {
		std::cout<< "usage: <file name> <hex value>" << std::endl;
		return -1;
	}

	char* s = argv[2];
	uint8_t pat_buf[256];
	int idx = 0;
	while (*s != 0) {
		uint8_t v = *s - '0';
		s++;
		pat_buf[idx] = (v << 4) + (*s - '0');
		s++;
		idx++;
	}

	int pat_len = idx;
	uint8_t* window;
	FileReader reader(argv[1]);
	while (reader.getBytes(&window, pat_len) == 0) {
		bool found = true;
		for(int i = 0; i < pat_len; i++) {
			if (window[i] != pat_buf[i]) {
				reader.advance(i+1);
				found = false;
				break;
			}
		}
		if (found) {
			reader.advance(pat_len);
			std::cout<<"found "<<reader.getCurPos()<<std::endl;
		}

	}



}
