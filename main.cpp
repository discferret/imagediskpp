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
	private:
		vector<IMDSector> _sectors;
		int _mode;
		int phys_cyl;
		int phys_head;
		int sec_sz;
	public:
		IMDTrack(istream &in)
		{
			// Mode value
			// Physical Cylinder
			// Head and flags
			// Number of Sectors
			// Sector Size Byte
			//
			// Sector Numbering Map
			// Optional Sector Cylinder Map
			// Optional Head Map
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
	ifstream f("01_Diagnosic_Disk_Ver_3.51.IMD", ios::in | ios::binary);

	IMDImage imd(f);
}

