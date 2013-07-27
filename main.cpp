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

/// Sector types
typedef enum {
	IMDS_NONE,			/// Sector data not available, couldn't be read
	IMDS_NORMAL,		/// Normal sector
	IMDS_DELETED,		/// Deleted-data address mark
	IMDS_NORMAL_DERR,	/// Normal sector read with data error
	IMDS_DELETED_DERR	/// Deleted sector read with data error
} IMDSectorType;

class IMDSector {
	public:
		vector<uint8_t> data;
		unsigned int logical_cylinder, logical_head, logical_sector;
		IMDSectorType type;

		IMDSector(istream &in, unsigned int cyl, unsigned int head, unsigned int sec, unsigned int ssz)
		{
			// Set the C/H/S values
			logical_cylinder = cyl;
			logical_head = head;
			logical_sector = sec;

			// Read and decode the sector format byte
			char b;
			bool is_compressed = false;
			in.read(&b, 1);
			switch (b) {
				case 0x00:		// Sector data unavailable - could not be read.
					type = IMDS_NONE;
					break;
				case 0x01:		// Normal data
					type = IMDS_NORMAL;
					break;
				case 0x02:		// Normal data -- all bytes have the same value (compressed)
					type = IMDS_NORMAL;
					is_compressed = true;
					break;
				case 0x03:		// Deleted data
					type = IMDS_DELETED;
					break;
				case 0x04:		// Deleted data -- all bytes have the same value
					type = IMDS_DELETED;
					is_compressed = true;
					break;
				case 0x05:		// Normal data read with data error
					type = IMDS_NORMAL_DERR;
					break;
				case 0x06:		// Normal data read with data error -- all with same value
					type = IMDS_NORMAL_DERR;
					is_compressed = true;
					break;
				case 0x07:		// Deleted data read with data error
					type = IMDS_DELETED_DERR;
					break;
				case 0x08:		// Deleted data read with data error -- all with same value
					type = IMDS_DELETED_DERR;
					is_compressed = true;
					break;
			}

			cout << "chs " << cyl << ":" << head << ":" << sec << " - " << ssz << " bytes, type " << type << ", " << (is_compressed ? "compressed" : "raw") << endl;

			// If there is no sector data, exit.
			if (type == IMDS_NONE) {
				return;
			}

			// Read the sector data
			if (!is_compressed) {
				// Uncompressed data
				char *x = new char[ssz];
				in.read(x, ssz);
				for (size_t i = 0; i < ssz; i++) {
					data.push_back((unsigned char)x[i]);
				}
				delete x;
			} else {
				// Compressed data -- all bytes in the sector have the same value
				in.read(&b, 1);
				for (size_t i = 0; i < ssz; i++) {
					data.push_back((unsigned char)b);
				}
			}
		}
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

			// Convert sector size into bytes
			size_t sector_bytes = (128 << sector_size);

			// Sector Data
			for (unsigned int x = 0; x < num_sectors; x++) {
				IMDSector s(in,
						has_scm ? sector_cyl_map[x] : phys_cyl,		// cylinder
						has_shm ? sector_head_map[x] : phys_head,	// head
						sector_num_map[x],							// sector
						sector_bytes);								// sector size in bytes
			}
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
		IMDImage(fstream &in)
		{
			// Get the file size
			in.seekp(0, ios::end);
			streampos fsize = in.tellp();
			in.seekp(0, ios::beg);

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
			do {
				IMDTrack t(in);
				_tracks.push_back(t);
			} while (in.tellp() < fsize);
		}
};

int main(void)
{
	fstream f("01_Diagnosic_Disk_Ver_3.51.IMD", ios::in | ios::binary);

	IMDImage imd(f);
}

