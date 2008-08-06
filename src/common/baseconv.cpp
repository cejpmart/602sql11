// baseconv.cpp
const uns8 csconv[256] = {
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 144,145,146,147,148,149,150,151,152,153,138,155,140,141,142,143,
 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 176,177,178,163,180,181,182,183,184,165,170,187,188,189,188,175,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,247,216,217,218,219,220,221,222,255};

const uns8 ansiconv[256] = {
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
//  96, 97, 98, 99,100,101,102,103,104,105,106,107,108,109,110,111,
// 112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
  96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,158,
 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,
 240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};

const uns8 conv1252[256] = {
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 144,145,146,147,148,149,150,151,152,153,138,155,140,157,142,159,
 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,247,216,217,218,219,220,221,222,158};

const uns8 conv8859_2[256] = {
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 176,161,178,163,180,165,166,183,184,169,170,171,172,189,174,175,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,247,216,217,218,219,220,221,222,255};

const uns8 conv8859_1[256] = {
   0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
  16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
  32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
  48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
  64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
  96, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
  80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,123,124,125,126,127,
 128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,
 144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,159,
 160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,
 176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
 192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,
 208,209,210,211,212,213,214,247,216,217,218,219,220,221,222,255};

const wchar_t unic_latin_2[256] = { 
  0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
  0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
  0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
  0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
  0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
  0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
  0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
  0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
  0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
  0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
  0x00A0,0x0104,0x02D8,0x0141,0x00A4,0x013D,0x015A,0x00A7,0x00A8,0x0160,0x015E,0x0164,0x0179,0x00AD,0x017D,0x017B,
  0x00B0,0x0105,0x02DB,0x0142,0x00B4,0x013E,0x015B,0x02C7,0x00B8,0x0161,0x015F,0x0165,0x017A,0x02DD,0x017E,0x017C,
  0x0154,0x00C1,0x00C2,0x0102,0x00C4,0x0139,0x0106,0x00C7,0x010C,0x00C9,0x0118,0x00CB,0x011A,0x00CD,0x00CE,0x010E,
  0x0110,0x0143,0x0147,0x00D3,0x00D4,0x0150,0x00D6,0x00D7,0x0158,0x016E,0x00DA,0x0170,0x00DC,0x00DD,0x0162,0x00DF,
  0x0155,0x00E1,0x00E2,0x0103,0x00E4,0x013A,0x0107,0x00E7,0x010D,0x00E9,0x0119,0x00EB,0x011B,0x00ED,0x00EE,0x010F,
  0x0111,0x0144,0x0148,0x00F3,0x00F4,0x0151,0x00F6,0x00F7,0x0159,0x016F,0x00FA,0x0171,0x00FC,0x00FD,0x0163,0x02D9
};

const wchar_t unic_latin_1[256] = { 
  0x0000,0x0001,0x0002,0x0003,0x0004,0x0005,0x0006,0x0007,0x0008,0x0009,0x000A,0x000B,0x000C,0x000D,0x000E,0x000F,
  0x0010,0x0011,0x0012,0x0013,0x0014,0x0015,0x0016,0x0017,0x0018,0x0019,0x001A,0x001B,0x001C,0x001D,0x001E,0x001F,
  0x0020,0x0021,0x0022,0x0023,0x0024,0x0025,0x0026,0x0027,0x0028,0x0029,0x002A,0x002B,0x002C,0x002D,0x002E,0x002F,
  0x0030,0x0031,0x0032,0x0033,0x0034,0x0035,0x0036,0x0037,0x0038,0x0039,0x003A,0x003B,0x003C,0x003D,0x003E,0x003F,
  0x0040,0x0041,0x0042,0x0043,0x0044,0x0045,0x0046,0x0047,0x0048,0x0049,0x004A,0x004B,0x004C,0x004D,0x004E,0x004F,
  0x0050,0x0051,0x0052,0x0053,0x0054,0x0055,0x0056,0x0057,0x0058,0x0059,0x005A,0x005B,0x005C,0x005D,0x005E,0x005F,
  0x0060,0x0061,0x0062,0x0063,0x0064,0x0065,0x0066,0x0067,0x0068,0x0069,0x006A,0x006B,0x006C,0x006D,0x006E,0x006F,
  0x0070,0x0071,0x0072,0x0073,0x0074,0x0075,0x0076,0x0077,0x0078,0x0079,0x007A,0x007B,0x007C,0x007D,0x007E,0x007F,
  0x0080,0x0081,0x0082,0x0083,0x0084,0x0085,0x0086,0x0087,0x0088,0x0089,0x008A,0x008B,0x008C,0x008D,0x008E,0x008F,
  0x0090,0x0091,0x0092,0x0093,0x0094,0x0095,0x0096,0x0097,0x0098,0x0099,0x009A,0x009B,0x009C,0x009D,0x009E,0x009F,
  0x00A0,0x00A1,0x00A2,0x00A3,0x00A4,0x00A5,0x00A6,0x00A7,0x00A8,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
  0x00B0,0x00B1,0x00B2,0x00B3,0x00B4,0x00B5,0x00B6,0x00B7,0x00B8,0x00B9,0x00BA,0x00BB,0x00BC,0x00BD,0x00BE,0x00BF,
  0x00C0,0x00C1,0x00C2,0x00C3,0x00C4,0x00C5,0x00C6,0x00C7,0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
  0x00D0,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x00DD,0x00DE,0x00DF,
  0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
  0x00F0,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x00FD,0x00FE,0x00FF
};

const uns8 c7_ansi[] =
{   0, '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '.', '_',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_',
  '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '_', '_', '_', '_',
  '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_', '_', '_', '_', '_',
  'C', 'u', 'e', 'a', 'a', 'a', 'a', 'c', 'e', 'e', 'e', 'i', 'i', 'i', 'A', 'A',
  'E', 'o', 'o', 'o', 'u', 'u', 'y', 'O', 'U', '_', '_', '_', '_', '_', '_', '_',
  'a', 'i', 'o', 'u', 'n', 'N', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_'
};

const uns8 c7_1250[] =
{   0, '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '.', '_',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_',
  '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '_', '_', '_', '_',
  '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', 'S', '_', 'S', 'T', 'Z', 'Z',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', 's', '_', 's', 't', 'z', 'z',
  '_', '_', '_', 'L', '_', 'A', '_', '_', '_', '_', 'S', '_', '_', '_', '_', 'Z',
  '_', '_', '_', 'l', '_', '_', '_', '_', '_', 'a', 's', '_', 'L', '_', 'l', 'z',
  'R', 'A', 'A', 'A', 'A', 'L', 'C', 'C', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'D',
  'D', 'N', 'N', 'O', 'O', 'O', 'O', '_', 'R', 'U', 'U', 'U', 'U', 'Y', 'T', 's',
  'r', 'a', 'a', 'a', 'a', 'l', 'c', 'c', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'd',
  'd', 'n', 'n', 'o', 'o', 'o', 'o', '_', 'r', 'u', 'u', 'u', 'u', 'y', 't', '_'
};

const uns8 c7_1252[] =
{   0, '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '.', '_',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_',
  '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '_', '_', '_', '_',
  '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', 'S', '_', '_', '_', 'Z', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', 's', '_', '_', '_', 'z', 'Y',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  'A', 'A', 'A', 'A', 'A', 'A', '_', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
  '_', 'N', 'O', 'O', 'O', 'O', 'O', '_', 'O', 'U', 'U', 'U', 'U', 'Y', '_', 's',
  'a', 'a', 'a', 'a', 'a', 'a', '_', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
  '_', 'n', 'o', 'o', 'o', 'o', 'o', '_', 'o', 'u', 'u', 'u', 'u', 'y', '_', 'y'
};

const uns8 c7_8859_2[] =
{   0, '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '.', '_',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_',
  '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '_', '_', '_', '_',
  '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', 'A', '_', 'L', '_', 'L', 'S', '_', '_', 'S', 'S', 'T', 'Z', '_', 'Z', 'Z',
  '_', 'a', '_', 'l', '_', 'l', 's', '_', '_', 's', 's', 't', 'z', '_', 'z', 'z',
  'R', 'A', 'A', 'A', 'A', 'L', 'C', 'C', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'D',
  'D', 'N', 'N', 'O', 'O', 'O', 'O', '_', 'R', 'U', 'U', 'U', 'U', 'Y', 'T', 's',
  'r', 'a', 'a', 'a', 'a', 'l', 'c', 'c', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'd',
  'd', 'n', 'n', 'o', 'o', 'o', 'o', '_', 'r', 'u', 'u', 'u', 'u', 'y', 't', '_'
};

const uns8 c7_8859_1[] =
{   0, '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '.', '_',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_',
  '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '_', '_', '_', '_', '_',
  '_', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_',
  'A', 'A', 'A', 'A', 'A', 'A', '_', 'C', 'E', 'E', 'E', 'E', 'I', 'I', 'I', 'I',
  'D', 'N', 'O', 'O', 'O', 'O', 'O', '_', 'O', 'U', 'U', 'U', 'U', 'Y', '_', 's',
  'a', 'a', 'a', 'a', 'a', 'a', '_', 'c', 'e', 'e', 'e', 'e', 'i', 'i', 'i', 'i',
  '_', 'n', 'o', 'o', 'o', 'o', 'o', '_', 'o', 'u', 'u', 'u', 'u', 'y', '_', 'y'
};

/////////// alphabetical ordering of Unicode characters ////////////////////////
#define ORA  0x41
#define ORB  0x42
#define ORC  0x43
#define ORD  0x44
#define ORE  0x45
#define ORF  0x46
#define ORG  0x47
#define ORH  0x48
#define ORCh 0x49
#define ORI  0x4A
#define ORJ  0x4B
#define ORK  0x4C
#define ORL  0x4D
#define ORM  0x4E
#define ORN  0x4F
#define ORO  0x50
#define ORP  0x51
#define ORQ  0x52
#define ORR  0x53
#define ORS  0x54
#define ORT  0x55
#define ORU  0x56
#define ORV  0x57
#define ORW  0x58
#define ORX  0x59
#define ORY  0x5A
#define ORZ  0x5B
#define ORETH    0x5C
#define ORTHORN  0x5D

#define ORSML 0x1000

#define OR_OCAR   1   // obracena carka
#define OR_CAR    2   // carka
#define OR_VOKAN  3
#define OR_TILDA  4
#define OR_DDOT   5   // dve tecky, tecka
#define OR_DCAR   6   // dve carky
#define OR_HACEK  7
#define OR_CEDIL  8   // i polske
#define OR_KROUZ  9
#define OR_SPEC  10   // scharfes S, preskrtle D, L, O; AE

#define OR_BIG(base,diacr) (16*base+diacr)
#define OR_SML(base,diacr) (16*base+diacr+ORSML)
#define OR_CHAR(base)      (16*base)          
#define ORZN 0x200
#define NONCHR  OR_CHAR(ORZN+200)
#define TERMCH  NONCHR+1  // terminator in index-passing without limit

uns16 unicode_order[] = {
              OR_BIG(ORA,0),OR_BIG(ORB,0),OR_BIG(ORC,0),OR_BIG(ORD,0),OR_BIG(ORE,0),OR_BIG(ORF,0),OR_BIG(ORG,0), // 40
OR_BIG(ORH,0),OR_BIG(ORI,0),OR_BIG(ORJ,0),OR_BIG(ORK,0),OR_BIG(ORL,0),OR_BIG(ORM,0),OR_BIG(ORN,0),OR_BIG(ORO,0),
OR_BIG(ORP,0),OR_BIG(ORQ,0),OR_BIG(ORR,0),OR_BIG(ORS,0),OR_BIG(ORT,0),OR_BIG(ORU,0),OR_BIG(ORV,0),OR_BIG(ORW,0),
OR_BIG(ORX,0),OR_BIG(ORY,0),OR_BIG(ORZ,0),OR_CHAR(ORZN+1),OR_CHAR(ORZN+2),OR_CHAR(ORZN+3),OR_CHAR(ORZN+4),OR_CHAR(ORZN+5),

OR_CHAR(ORZN+6),OR_SML(ORA,0),OR_SML(ORB,0),OR_SML(ORC,0),OR_SML(ORD,0),OR_SML(ORE,0),OR_SML(ORF,0),OR_SML(ORG,0), // 60
OR_SML(ORH,0),OR_SML(ORI,0),OR_SML(ORJ,0),OR_SML(ORK,0),OR_SML(ORL,0),OR_SML(ORM,0),OR_SML(ORN,0),OR_SML(ORO,0),
OR_SML(ORP,0),OR_SML(ORQ,0),OR_SML(ORR,0),OR_SML(ORS,0),OR_SML(ORT,0),OR_SML(ORU,0),OR_SML(ORV,0),OR_SML(ORW,0),
OR_SML(ORX,0),OR_SML(ORY,0),OR_SML(ORZ,0),OR_CHAR(ORZN+7),OR_CHAR(ORZN+8),OR_CHAR(ORZN+9),OR_CHAR(ORZN+10),OR_CHAR(ORZN+11),

OR_CHAR(ORZN+100),OR_CHAR(ORZN+101),OR_CHAR(ORZN+102),OR_CHAR(ORZN+103),OR_CHAR(ORZN+104),OR_CHAR(ORZN+105),OR_CHAR(ORZN+106),OR_CHAR(ORZN+107),          // 80
OR_CHAR(ORZN+110),OR_CHAR(ORZN+111),OR_CHAR(ORZN+112),OR_CHAR(ORZN+113),OR_CHAR(ORZN+114),OR_CHAR(ORZN+115),OR_CHAR(ORZN+116),OR_CHAR(ORZN+117),          // 88
OR_CHAR(ORZN+120),OR_CHAR(ORZN+121),OR_CHAR(ORZN+122),OR_CHAR(ORZN+123),OR_CHAR(ORZN+124),OR_CHAR(ORZN+125),OR_CHAR(ORZN+126),OR_CHAR(ORZN+127),          // 90
OR_CHAR(ORZN+130),OR_CHAR(ORZN+131),OR_CHAR(ORZN+132),OR_CHAR(ORZN+133),OR_CHAR(ORZN+134),OR_CHAR(ORZN+135),OR_CHAR(ORZN+136),OR_CHAR(ORZN+137),          // 98
OR_CHAR(ORZN+140),OR_CHAR(ORZN+141),OR_CHAR(ORZN+142),OR_CHAR(ORZN+143),OR_CHAR(ORZN+144),OR_CHAR(ORZN+145),OR_CHAR(ORZN+146),OR_CHAR(ORZN+147),          // a0
OR_CHAR(ORZN+150),OR_CHAR(ORZN+151),OR_CHAR(ORZN+152),OR_CHAR(ORZN+153),OR_CHAR(ORZN+154),OR_CHAR(ORZN+155),OR_CHAR(ORZN+156),OR_CHAR(ORZN+157),          // a8
OR_CHAR(ORZN+160),OR_CHAR(ORZN+161),OR_CHAR(ORZN+162),OR_CHAR(ORZN+163),OR_CHAR(ORZN+164),OR_CHAR(ORZN+165),OR_CHAR(ORZN+166),OR_CHAR(ORZN+167),          // b0
OR_CHAR(ORZN+170),OR_CHAR(ORZN+171),OR_CHAR(ORZN+172),OR_CHAR(ORZN+173),OR_CHAR(ORZN+174),OR_CHAR(ORZN+175),OR_CHAR(ORZN+176),OR_CHAR(ORZN+177),          // b8

OR_BIG(ORA,OR_OCAR),OR_BIG(ORA,OR_CAR),OR_BIG(ORA,OR_VOKAN),OR_BIG(ORA,OR_TILDA), OR_BIG(ORA,OR_DDOT),OR_BIG(ORA,OR_KROUZ),OR_BIG(ORA,OR_SPEC),OR_BIG(ORC,OR_CEDIL), // C0
OR_BIG(ORE,OR_OCAR),OR_BIG(ORE,OR_CAR),OR_BIG(ORE,OR_VOKAN),OR_BIG(ORE,OR_DDOT),  OR_BIG(ORI,OR_OCAR),OR_BIG(ORI,OR_CAR),  OR_BIG(ORI,OR_VOKAN),OR_BIG(ORI,OR_DDOT), // C8
OR_BIG(ORETH,0),    OR_BIG(ORN,OR_TILDA),OR_BIG(ORO,OR_OCAR),OR_BIG(ORO,OR_CAR),  OR_BIG(ORO,OR_VOKAN),OR_BIG(ORO,OR_TILDA),OR_BIG(ORO,OR_DDOT),   OR_CHAR(ORZN+180), // D0
OR_BIG(ORO,OR_SPEC),OR_BIG(ORU,OR_OCAR),OR_BIG(ORU,OR_CAR),OR_BIG(ORU,OR_VOKAN),  OR_BIG(ORU,OR_DDOT),OR_BIG(ORY,OR_CAR),  OR_BIG(ORTHORN,0), OR_SML(ORS,OR_SPEC),    // D8

OR_SML(ORA,OR_OCAR),OR_SML(ORA,OR_CAR),OR_SML(ORA,OR_VOKAN),OR_SML(ORA,OR_TILDA), OR_SML(ORA,OR_DDOT),OR_SML(ORA,OR_KROUZ),OR_SML(ORA,OR_SPEC),OR_SML(ORC,OR_CEDIL), // E0
OR_SML(ORE,OR_OCAR),OR_SML(ORE,OR_CAR),OR_SML(ORE,OR_VOKAN),OR_SML(ORE,OR_DDOT),  OR_SML(ORI,OR_OCAR),OR_SML(ORI,OR_CAR),  OR_SML(ORI,OR_VOKAN),OR_SML(ORI,OR_DDOT), // E8
OR_SML(ORETH,0),    OR_SML(ORN,OR_TILDA),OR_SML(ORO,OR_OCAR),OR_SML(ORO,OR_CAR),  OR_SML(ORO,OR_VOKAN),OR_SML(ORO,OR_TILDA),OR_SML(ORO,OR_DDOT),   OR_CHAR(ORZN+181), // F0
OR_SML(ORO,OR_SPEC),OR_SML(ORU,OR_OCAR),OR_SML(ORU,OR_CAR),OR_SML(ORU,OR_VOKAN),  OR_SML(ORU,OR_DDOT),OR_SML(ORY,OR_CAR),  OR_SML(ORTHORN,0), OR_SML(ORY,OR_DDOT),    // F8

NONCHR,  NONCHR,  OR_BIG(ORA,OR_HACEK),OR_SML(ORA,OR_HACEK),                            OR_BIG(ORA,OR_CEDIL),OR_SML(ORA,OR_CEDIL),OR_BIG(ORC,OR_CAR),OR_SML(ORC,OR_CAR),  // 100
NONCHR,  NONCHR, NONCHR,  NONCHR,                                                           OR_BIG(ORC,OR_HACEK),OR_SML(ORC,OR_HACEK),OR_BIG(ORD,OR_HACEK),OR_SML(ORD,OR_HACEK), // 108
OR_BIG(ORD,OR_SPEC),OR_SML(ORD,OR_SPEC), NONCHR,  NONCHR,                                NONCHR,  NONCHR,  NONCHR,  NONCHR,   // 110
OR_BIG(ORE,OR_CEDIL),OR_SML(ORE,OR_CEDIL),OR_BIG(ORE,OR_HACEK),OR_SML(ORE,OR_HACEK),   NONCHR,  NONCHR, NONCHR,  NONCHR,// 118
NONCHR,  NONCHR,  NONCHR,  NONCHR,                                                       NONCHR,  NONCHR,  NONCHR,  NONCHR,  // 120
NONCHR,  NONCHR,  NONCHR,  NONCHR,                                                       NONCHR,  NONCHR,  NONCHR,  NONCHR,  // 128
NONCHR,  NONCHR,  NONCHR,  NONCHR,                                                       NONCHR,  NONCHR,  NONCHR,  NONCHR,  // 130
NONCHR,  OR_BIG(ORL,OR_CAR),OR_SML(ORL,OR_CAR),  NONCHR,                             NONCHR,  OR_BIG(ORL,OR_HACEK),OR_SML(ORL,OR_HACEK), NONCHR,  // 138
NONCHR,  OR_BIG(ORL,OR_SPEC),OR_SML(ORL,OR_SPEC),  OR_BIG(ORN,OR_CAR),             OR_SML(ORL,OR_CAR),  NONCHR,  NONCHR,  OR_BIG(ORN,OR_HACEK),  // 140
OR_SML(ORN,OR_HACEK),  NONCHR,  NONCHR,  NONCHR,                                       NONCHR,  NONCHR,  NONCHR,  NONCHR,  // 148
OR_BIG(ORO,OR_DCAR),OR_SML(ORO,OR_DCAR),  NONCHR,  NONCHR,                           OR_BIG(ORR,OR_CAR),OR_SML(ORR,OR_CAR),NONCHR,  NONCHR,  // 150
OR_BIG(ORR,OR_HACEK),OR_SML(ORR,OR_HACEK),OR_BIG(ORS,OR_CAR),OR_SML(ORS,OR_CAR), NONCHR,  NONCHR,  OR_BIG(ORS,OR_CEDIL),OR_SML(ORS,OR_CEDIL),  // 158
OR_BIG(ORS,OR_HACEK),OR_SML(ORS,OR_HACEK),OR_BIG(ORT,OR_CEDIL),OR_SML(ORT,OR_CEDIL), OR_BIG(ORT,OR_HACEK),OR_SML(ORT,OR_HACEK), NONCHR,  NONCHR,  // 160
NONCHR,  NONCHR,   NONCHR,  NONCHR,                                                       NONCHR,  NONCHR,  OR_BIG(ORU,OR_KROUZ),OR_SML(ORU,OR_KROUZ),  // 168
OR_BIG(ORU,OR_DCAR),OR_SML(ORU,OR_DCAR), NONCHR,  NONCHR,                            NONCHR,  NONCHR,  NONCHR,  NONCHR,  // 170
NONCHR,  OR_BIG(ORZ,OR_CAR),OR_SML(ORZ,OR_CAR),  OR_BIG(ORZ,OR_DDOT),              OR_SML(ORZ,OR_DDOT),  OR_BIG(ORZ,OR_HACEK),OR_SML(ORZ,OR_HACEK)   // 178
};

CFNC int WINAPI general_comp_strA(const char * s1, const char * s2, unsigned maxlen, t_specif spec)
{ wbcharset_t charset(spec.charset);
  int small_change = 0;  // the 1st small change
  int maxlen1 = maxlen, maxlen2 = maxlen;
  while (maxlen1-- && maxlen2--)
  { wchar_t u1, u2;
	if (spec.ignore_case)
	{ u1 = charset.unic_table()[charset.case_table()[(uns8)*s1]];
	  u2 = charset.unic_table()[charset.case_table()[(uns8)*s2]];
	}
	else
	{ u1 = charset.unic_table()[(uns8)*s1];
	  u2 = charset.unic_table()[(uns8)*s2];
	}
	uns16 ord1, ord2;
	ord1 = u1<'A' ? 16*u1 : (u1-'A')<sizeof(unicode_order)/sizeof(uns16) ? unicode_order[u1-'A'] : TERMCH;
	ord2 = u2<'A' ? 16*u2 : (u2-'A')<sizeof(unicode_order)/sizeof(uns16) ? unicode_order[u2-'A'] : TERMCH;
	// Ch:
	if ((spec.charset & 0x7f) == 1)
	{ if (*s1=='c' || *s1=='C')
		if (maxlen1)
		  if (s1[1]=='h' || s1[1]=='H')
		  { ord1 = *s1=='C' || spec.ignore_case ? OR_BIG(ORCh,0) : OR_SML(ORCh,0);
		    s1++;  maxlen1--; 
		  }
	  if (*s2=='c' || *s2=='C')
		if (maxlen2)
		  if (s2[1]=='h' || s2[1]=='H')
		  { ord2 = *s2=='C' || spec.ignore_case ? OR_BIG(ORCh,0) : OR_SML(ORCh,0);
		    s2++;  maxlen2--; 
		  }
	}
	// compare the ordering:
	if ((ord1 >> 4) < (ord2 >> 4)) return -1;
	if ((ord1 >> 4) > (ord2 >> 4)) return 1;
	if (!small_change)
	if (ord1<ord2) small_change=-1;
	else if (ord1>ord2) small_change=1;
    if (!*s1) break;
    s1++;  s2++;
  }
  return small_change;
}

CFNC int WINAPI general_comp_strW(const wuchar * s1, const wuchar * s2, unsigned maxlen, t_specif spec)
{ wbcharset_t charset(spec.charset);
  int small_change = 0;  // the 1st small change
  int maxlen1 = maxlen, maxlen2 = maxlen;
  while (maxlen1-- && maxlen2--)
  { wchar_t u1, u2;
	if (spec.ignore_case)
	{ u1 = upcase_charW(*s1, charset);
	  u2 = upcase_charW(*s2, charset);
	}
	else
	{ u1 = *s1;
	  u2 = *s2;
	}
	uns16 ord1, ord2;
	ord1 = u1<'A' ? 16*u1 : (u1-'A')<sizeof(unicode_order)/sizeof(uns16) ? unicode_order[u1-'A'] : TERMCH;
	ord2 = u2<'A' ? 16*u2 : (u2-'A')<sizeof(unicode_order)/sizeof(uns16) ? unicode_order[u2-'A'] : TERMCH;
	// Ch:
	if ((spec.charset & 0x7f) == 1)
	{ if (*s1=='c' || *s1=='C')
		if (maxlen1)
		  if (s1[1]=='h' || s1[1]=='H')
		  { ord1 = *s1=='C' || spec.ignore_case ? OR_BIG(ORCh,0) : OR_SML(ORCh,0);
			s1++;  maxlen1--; 
		  }
	if (*s2=='c' || *s2=='C')
		if (maxlen2)
		  if (s2[1]=='h' || s2[1]=='H')
		  { ord2 = *s2=='C' || spec.ignore_case ? OR_BIG(ORCh,0) : OR_SML(ORCh,0);
			s2++;  maxlen2--; 
		  }
	}
	// compare the ordering:
	if ((ord1 >> 4) < (ord2 >> 4)) return -1;
	if ((ord1 >> 4) > (ord2 >> 4)) return 1;
	if (!small_change)
	if (ord1<ord2) small_change=-1;
	else if (ord1>ord2) small_change=1;
    if (!*s1) break;
    s1++;  s2++;
  }
  return small_change;
}

const uns8 * wbcharset_t::case_tables_win[] =
{ ansiconv,
  csconv,
  csconv,
  conv1252,
  conv1252,
  conv1252
};

const uns8 * wbcharset_t::case_tables_iso[] =
{ ansiconv,
  conv8859_2,
  conv8859_2,
  conv8859_1,
  conv8859_1,
  conv8859_1
};

const wchar_t * wbcharset_t::unic_tables_iso[] =
{ NULL,
  unic_latin_2,
  unic_latin_2,
  unic_latin_1,
  unic_latin_1,
  unic_latin_1
};

const uns8 * wbcharset_t::c7_tables_win[] =
{ c7_ansi,
  c7_1250,
  c7_1250,
  c7_1252,
  c7_1252,
  c7_1252
};

const uns8 * wbcharset_t::c7_tables_iso[] =
{ c7_ansi,
  c7_8859_2,
  c7_8859_2,
  c7_8859_1,
  c7_8859_1,
  c7_8859_1
};

#ifdef WINS
DWORD wbcharset_t::lcid_table[CHARSET_COUNT] =
{ MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT), 
  MAKELCID(MAKELANGID(LANG_CZECH,   SUBLANG_DEFAULT), SORT_DEFAULT), 
  MAKELCID(MAKELANGID(LANG_POLISH,  SUBLANG_DEFAULT), SORT_DEFAULT), 
  MAKELCID(MAKELANGID(LANG_FRENCH,  SUBLANG_FRENCH),  SORT_DEFAULT), 
  MAKELCID(MAKELANGID(LANG_GERMAN,  SUBLANG_GERMAN),  SORT_DEFAULT), 
  MAKELCID(MAKELANGID(LANG_ITALIAN, SUBLANG_ITALIAN), SORT_DEFAULT),
  MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT), // not used 
  MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), SORT_DEFAULT)  // not used 
};
int   wbcharset_t::  cp_table_win[CHARSET_COUNT] =
 { CP_ACP, 1250,  1250,  1252,  1252,  1252,  852, CP_UTF8  };
int   wbcharset_t::  cp_table_iso[CHARSET_COUNT] =
 { CP_ACP, 28592, 28592, 28591, 28591, 28591, 0,   0 };
#else

#ifndef LC_ALL_MASK  // not defined on RH 7.3
# define LC_CTYPE_MASK		(1 << __LC_CTYPE)
# define LC_NUMERIC_MASK	(1 << __LC_NUMERIC)
# define LC_TIME_MASK		(1 << __LC_TIME)
# define LC_COLLATE_MASK	(1 << __LC_COLLATE)
# define LC_MONETARY_MASK	(1 << __LC_MONETARY)
# define LC_MESSAGES_MASK	(1 << __LC_MESSAGES)
# define LC_PAPER_MASK		(1 << __LC_PAPER)
# define LC_NAME_MASK		(1 << __LC_NAME)
# define LC_ADDRESS_MASK	(1 << __LC_ADDRESS)
# define LC_TELEPHONE_MASK	(1 << __LC_TELEPHONE)
# define LC_MEASUREMENT_MASK	(1 << __LC_MEASUREMENT)
# define LC_IDENTIFICATION_MASK	(1 << __LC_IDENTIFICATION)
# define LC_ALL_MASK		(LC_CTYPE_MASK \
				 | LC_NUMERIC_MASK \
				 | LC_TIME_MASK \
				 | LC_COLLATE_MASK \
				 | LC_MONETARY_MASK \
				 | LC_MESSAGES_MASK \
				 | LC_PAPER_MASK \
				 | LC_NAME_MASK \
				 | LC_ADDRESS_MASK \
				 | LC_TELEPHONE_MASK \
				 | LC_MEASUREMENT_MASK \
				 | LC_IDENTIFICATION_MASK \
				 )
#endif

const char * locale_name_iso[CHARSET_COUNT] =
{ "C", "cs_CZ.ISO-8859-2", "pl_PL.ISO-8859-2",
       "fr_FR.ISO-8859-1", "de_DE.ISO-8859-1", "it_IT.ISO-8859-1",
  "C", "C"
};

const char * locale_name_win[CHARSET_COUNT] =
{ "C", "cs_CZ.CP1250", "pl_PL.CP1250",
       "fr_FR.CP1252", "de_DE.CP1252", "it_IT.CP1252",
  "C", "C"
};

charset_t wbcharset_t::code_table_iso[CHARSET_COUNT] =
 { __newlocale(LC_ALL_MASK, locale_name_iso[0], NULL),
   __newlocale(LC_ALL_MASK, locale_name_iso[1], NULL),
   __newlocale(LC_ALL_MASK, locale_name_iso[2], NULL),
   __newlocale(LC_ALL_MASK, locale_name_iso[3], NULL),
   __newlocale(LC_ALL_MASK, locale_name_iso[4], NULL),
   __newlocale(LC_ALL_MASK, locale_name_iso[5], NULL),
   __newlocale(LC_ALL_MASK, locale_name_iso[6], NULL),
   __newlocale(LC_ALL_MASK, locale_name_iso[7], NULL)
 };

charset_t wbcharset_t::code_table_win[CHARSET_COUNT] =
 { __newlocale(LC_ALL_MASK, locale_name_win[0], NULL),
   __newlocale(LC_ALL_MASK, locale_name_win[1], NULL),
   __newlocale(LC_ALL_MASK, locale_name_win[2], NULL),
   __newlocale(LC_ALL_MASK, locale_name_win[3], NULL),
   __newlocale(LC_ALL_MASK, locale_name_win[4], NULL),
   __newlocale(LC_ALL_MASK, locale_name_win[5], NULL),
   __newlocale(LC_ALL_MASK, locale_name_win[6], NULL),
   __newlocale(LC_ALL_MASK, locale_name_win[7], NULL)
 };

charset_t wbcharset_t::locales8(void)
{ if (!(code & 0x80) && code >= CHARSET_COUNT) return code_table_iso[0];
  charset_t ch = (code & 0x80) ? code_table_iso[code & 0x7f] : code_table_win[code & 0x7f];
  if (!ch)
  { 
#ifdef SRVR
    char msg[100];
    sprintf(msg, "Locale %s is not available!", (code & 0x80) ? locale_name_iso[code & 0x7f] : locale_name_win[code]);
    dbg_err(msg);
#endif
    ch=code_table_iso[0];
  }
  return ch;
}

charset_t wbcharset_t::locales16(void)
{ if (!(code & 0x80) && code >= CHARSET_COUNT) return code_table_iso[0];
  charset_t ch;
  ch = code_table_iso[code & 0x7f];
  if (!ch) ch=code_table_win[code & 0x7f];
  if (!ch)
  { 
#ifdef SRVR
    char msg[100];
    sprintf(msg, "Locale %s is not available!", (code & 0x80) ? locale_name_iso[code & 0x7f] : locale_name_win[code]);
    dbg_err(msg);
#endif
    ch=code_table_iso[0];
  }
  return ch;
}

#include <iconv.h>
bool wbcharset_t::prepare_conversions(void)
{ iconv_t conv = iconv_open("UCS-2", code_name_iso[0]);
  if (conv==(iconv_t)-1)  // initial version falied, try others
  { const char * name = "ISO646-US";
    conv = iconv_open("UCS-2", name);
    if (conv==(iconv_t)-1)
    { name = "ISO646";
      conv = iconv_open("UCS-2", name);
      if (conv==(iconv_t)-1) return false;
    }
    code_name_iso[0]=name;
    code_name_win[0]=name;
  }
  iconv_close(conv);
  return true;
}

#endif  // LINUX

const char* wbcharset_t::code_name_iso[] = {
  "ISO646.BASIC",
  "ISO-8859-2",
  "ISO-8859-2", // PL
  "ISO-8859-1", // FR
  "ISO-8859-1", // DE
  "ISO-8859-1",  // IT
  "852",
  "UTF8"
};
const char* wbcharset_t::code_name_win[] = {
  "ISO646.BASIC",
  "CP1250",
  "CP1250", // PL
  "CP1252", // FR
  "CP1252", // DE
  "CP1252", // IT
  "852",
  "UTF8"
};

t_charset_data wbcharset_t::charset_data_win[CHARSET_COUNT] =
{ { NULL} , { NULL}, { NULL}, { NULL}, { NULL}, { NULL}, { NULL}, { NULL} }; 
t_charset_data wbcharset_t::charset_data_iso[CHARSET_COUNT] =
{ { NULL} , { NULL}, { NULL}, { NULL}, { NULL}, { NULL}, { NULL}, { NULL} }; 

#ifdef SRVR
#ifdef LINUX
extern "C" void pcre_set_locale(struct __locale_struct * locIn);
#endif

void init_charset_data(void)
{ 
#if WBVERS>=950
  char * p;
#ifdef WINS
  p = setlocale(LC_CTYPE, "Czech_Czech Republic.1250");
  if (p) wbcharset_t::charset_data_win[1].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "Polish_Poland.1250");
  if (p) wbcharset_t::charset_data_win[2].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "French_France.1252");
  if (p) wbcharset_t::charset_data_win[3].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "German_Germany.1252");
  if (p) wbcharset_t::charset_data_win[4].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "Italian_Italy.1252");
  if (p) wbcharset_t::charset_data_win[5].pcre_table = pcre_maketables();

  p = setlocale(LC_CTYPE, "Czech_Czech Republic.28592");
  if (p) wbcharset_t::charset_data_iso[1].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "Polish_Poland.28592");
  if (p) wbcharset_t::charset_data_iso[2].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "French_France.28591");
  if (p) wbcharset_t::charset_data_iso[3].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "German_Germany.28591");
  if (p) wbcharset_t::charset_data_iso[4].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, "Italian_Italy.28591");
  if (p) wbcharset_t::charset_data_iso[5].pcre_table = pcre_maketables();
#else
  p = setlocale(LC_CTYPE, locale_name_win[1]);
  pcre_set_locale(wbcharset_t::code_table_win[1]);
  if (p) wbcharset_t::charset_data_win[1].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_win[2]);
  pcre_set_locale(wbcharset_t::code_table_win[2]);
  if (p) wbcharset_t::charset_data_win[2].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_win[3]);
  pcre_set_locale(wbcharset_t::code_table_win[3]);
  if (p) wbcharset_t::charset_data_win[3].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_win[4]);
  pcre_set_locale(wbcharset_t::code_table_win[4]);
  if (p) wbcharset_t::charset_data_win[4].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_win[5]);
  pcre_set_locale(wbcharset_t::code_table_win[5]);
  if (p) wbcharset_t::charset_data_win[5].pcre_table = pcre_maketables();

  p = setlocale(LC_CTYPE, locale_name_iso[1]);
  pcre_set_locale(wbcharset_t::code_table_iso[1]);
  if (p) wbcharset_t::charset_data_iso[1].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_iso[2]);
  pcre_set_locale(wbcharset_t::code_table_iso[2]);
  if (p) wbcharset_t::charset_data_iso[2].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_iso[3]);
  pcre_set_locale(wbcharset_t::code_table_iso[3]);
  if (p) wbcharset_t::charset_data_iso[3].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_iso[4]);
  pcre_set_locale(wbcharset_t::code_table_iso[4]);
  if (p) wbcharset_t::charset_data_iso[4].pcre_table = pcre_maketables();
  p = setlocale(LC_CTYPE, locale_name_iso[5]);
  pcre_set_locale(wbcharset_t::code_table_iso[5]);
  if (p) wbcharset_t::charset_data_iso[5].pcre_table = pcre_maketables();
#endif
  setlocale(LC_ALL, "");
#endif
}
#endif // SRVR

bool wbcharset_t::charset_available(void) const
// not testing Unicode comparison, not available on W98!
{
#ifdef LINUX
  if (code!=1)
  { charset_t ch = (code & 0x80) ? code_table_iso[code & 0x7f] : code_table_win[code & 0x7f];
    if (!ch)
      return false;
  }    
  iconv_t conv = iconv_open("UCS-2", (code & 0x80) ? code_name_iso[code & 0x7f] : code_name_win[code & 0x7f]);
  if (conv==(iconv_t)-1) 
    return false;
  iconv_close(conv);
#else
  char ch='a', res;
  if (!LCMapStringA(lcid(), LCMAP_UPPERCASE, &ch, 1, &res, 1))
    return false;
  if (!CompareStringA(lcid(), NORM_IGNORECASE, "a", -1, "b", -1))
    return false;
  wchar_t wch;
  if (!MultiByteToWideChar(cp(), 0, "a", 1, &wch, 1))
    return false;
#endif
  return true;
}

#ifdef WINS

wbcharset_t wbcharset_t::get_host_charset()
{ LANGID lid = GetUserDefaultLangID();
  for (int i = 0; i < sizeof(lcid_table) / sizeof(DWORD); i++)
    if (LOWORD(lcid_table[i]) == lid)
      return wbcharset_t(i);
  return charset_ansi;
}

#else

wbcharset_t wbcharset_t::get_host_charset()
{ char *lid = nl_langinfo(CODESET);
  for (int i = 0; i < sizeof(code_name_iso) / sizeof(const char *); i++)
    if (strcasecmp(code_name_iso[i], lid) == 0)
      return wbcharset_t(i);
  return wbcharset_t(CHARSET_NUM_UTF8);  // e.g. UTF-8 is not recorgnized without this
}

#endif
