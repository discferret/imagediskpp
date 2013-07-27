/**
 * @file
 * @author Philip Pemberton
 *
 * ImageDisk loader module.
 */

#include <cstdio>
#include <cstdint>
#include <vector>
#include <iostream>
#include <fstream>
#include <exception>
#include <string>
#include <cctype>

using namespace std;

class IMDSector {
	public:
		vector<uint8_t> _data;
		unsigned int logical_sector, logical_cylinder, logical_head;
};

class IMDTrack {
	public:
		vector<IMDSector> sectors;
		unsigned int mode;
		unsigned int phys_cyl;
		unsigned int phys_head;
		unsigned int sector_size;

		IMDTrack(istream &in)
		{
			char b;
			bool has_scm, has_shm;
			unsigned int num_sectors;

			// Mode value
			in.read(&b, 1); mode = (unsigned char)b;

			// Physical Cylinder
			in.read(&b, 1); phys_cyl = (unsigned char)b;

			// Head and flags
			// The actual head number can only be zero or one; the remaining
			// bits are used for flags.
			in.read(&b, 1); phys_head = (unsigned char)b & 1;
			has_scm = (b & 0x80);
			has_shm = (b & 0x40);

			// Number of Sectors
			in.read(&b, 1); num_sectors = (unsigned char)b;

			// Sector Size Byte
			in.read(&b, 1); sector_size = (unsigned char)b;

			// Sector Numbering Map
			char *sector_num_map = new char[num_sectors];
			in.read(sector_num_map, num_sectors);

			// Optional Sector Cylinder Map
			char *sector_cyl_map = NULL;
			if (has_scm) {
				sector_cyl_map = new char[num_sectors];
				in.read(sector_cyl_map, num_sectors);
			}

			// Optional Head Map
			char *sector_head_map = NULL;
			if (has_shm) {
				sector_head_map = new char[num_sectors];
				in.read(sector_head_map, num_sectors);
			}

			// Sector Data
		}
};

class EIMDNotValid : public exception
{
};

class IMDImage {
	private:
		vector<IMDTrack> _tracks;
		string _header;
		string _comment;
	public:
		IMDImage(istream &in)
		{
			// IMD files start with an "IMD v.vv: " header
			std::getline(in, _header);
			if ((_header.compare(0, 4, "IMD ") != 0) ||
					(_header[5] != '.') || (_header[8] != ':') || (_header[9] != ' ') ||
					!isdigit(_header[4]) || !isdigit(_header[6]) || !isdigit(_header[7])
			   ) {
				throw EIMDNotValid();
			} else {
				// TODO: decode version number and date
				cout << "IMD valid --> " << _header << endl;
			}

			// If the header is valid, it's fair to assume we have an IMD file. Next read the comment.
			std::getline(in, _comment, '\x1A');
			cout << "IMD comment: [" << _comment << "]" << endl;

			// Repeat for every track in the image...
			while (!in.eof()) {
				IMDTrack t(in);
				_tracks.push_back(t);
			}
		}
};

int main(void)
{
	fstream f("01_Diagnosic_Disk_Ver_3.51.IMD", ios::in | ios::binary);

	IMDImage imd(f);
}

