/*Ocean (Jonathan Dunn) (GB/GBC) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
long seqPtrLoc;
long seqOffset;
int i, j;
char outfile[1000000];
int format = 0;
int cmdFmt = 0;
int songNum;
long seqPtrs[4];
long songPtr;
long bankAmt;
int seqDiff = 0;
int foundTable = 0;
int songTempo = 0;
int highestSeq = 0;

unsigned long seqList[500];
unsigned static char* romData;
int totalSeqs;

const char Table1FindA[7] = { 0x26, 0x00, 0x29, 0x29, 0x29, 0x7D, 0xC6 };
const char Table1FindB[10] = { 0x26, 0x00, 0x5D, 0x54, 0x29, 0x29, 0x29, 0x19, 0x7D, 0xC6 };
const char Table1FindC[7] = { 0x26, 0x00, 0x29, 0x29, 0x29, 0x7D, 0x11 };
const char Table1FindD[4] = { 0x29, 0x19, 0x7D, 0xC6 };
const char Table2FindA[6] = { 0x2B, 0x72, 0x2B, 0x73, 0x87, 0xC6 };
const char Table2FindB[8] = { 0x2B, 0x72, 0x2B, 0x73, 0x5F, 0x16, 0x00, 0x21 };
const char Table2FindC[5] = { 0x2B, 0x72, 0x2B, 0x73, 0x11 };
const char Table2FindD[9] = { 0x2B, 0x72, 0x2B, 0x73, 0x26, 0x00, 0x6F, 0x29, 0x11 };
const char Table2FindE[7] = { 0x2B, 0x72, 0x2B, 0x73, 0x87, 0x5F, 0xFA };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptrs[4]);
void nutzSong2txt(int songNum, long ptr, int tempo);
void getSeqList(unsigned long list[], long offset);
void seqs2txt(unsigned long list[]);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("Ocean (Jonathan Dunn) (GB/GBC) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: OC2TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			if ((rom = fopen(argv[1], "rb")) == NULL)
			{
				printf("ERROR: Unable to open file %s!\n", argv[1]);
				exit(1);
			}
			else
			{
				bank = strtol(argv[2], NULL, 16);
				if (bank != 1)
				{
					bankAmt = bankSize;
				}
				else
				{
					bankAmt = 0;
				}
			}
			fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);
			fclose(rom);

			/*Try to search the bank for song pattern table loader*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], Table1FindB, 10)) && foundTable == 0)
				{
					tablePtrLoc = bankAmt + i + 10;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = romData[tablePtrLoc - bankAmt] + (romData[tablePtrLoc + 4 - bankAmt] * 0x100);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					format = 2;
					cmdFmt = 1;
					foundTable = 1;
					break;
				}
			}

			/*Try to search the bank for song pattern table loader*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], Table1FindC, 7)) && foundTable == 0)
				{
					tablePtrLoc = bankAmt + i + 7;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					format = 1;
					cmdFmt = 1;
					foundTable = 1;
					break;
				}
			}

			/*Try to search the bank for song pattern table loader*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], Table1FindD, 4)) && foundTable == 0)
				{
					tablePtrLoc = bankAmt + i + 4;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = romData[tablePtrLoc - bankAmt] + (romData[tablePtrLoc + 4 - bankAmt] * 0x100);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					format = 4;
					cmdFmt = 4;
					foundTable = 1;
					break;
				}
			}

			/*Try to search the bank for song pattern table loader*/
			if (format != 2)
			{
				for (i = 0; i < bankSize; i++)
				{
					if ((!memcmp(&romData[i], Table1FindA, 7)) && foundTable == 0)
					{
						tablePtrLoc = bankAmt + i + 7;
						printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
						tableOffset = romData[tablePtrLoc - bankAmt] + (romData[tablePtrLoc + 4 - bankAmt] * 0x100);
						printf("Song table starts at 0x%04x...\n", tableOffset);
						format = 1;
						foundTable = 1;
						break;
					}
				}
			}


			/*Now try to search the bank for sequence table loader*/

			/*Method 1 - RoboCop*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], Table2FindA, 6))
				{
					seqPtrLoc = bankAmt + i + 6;
					printf("Found pointer to sequence table at address 0x%04x!\n", seqPtrLoc);
					seqOffset = romData[seqPtrLoc - bankAmt] + (romData[seqPtrLoc + 3 - bankAmt] * 0x100);
					printf("Sequence table starts at 0x%04x...\n", seqOffset);
					if (cmdFmt == 0)
					{
						cmdFmt = 1;
					}
					break;

				}
			}

			/*Method 2 - RoboCop 2*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], Table2FindB, 8))
				{
					seqPtrLoc = bankAmt + i + 8;
					printf("Found pointer to sequence table at address 0x%04x!\n", seqPtrLoc);
					seqOffset = ReadLE16(&romData[seqPtrLoc - bankAmt]);
					printf("Sequence table starts at 0x%04x...\n", seqOffset);
					cmdFmt = 2;
					seqDiff = 1;
					break;

				}
			}

			/*Method 3 - Beauty and the Beast*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], Table2FindC, 5))
				{
					seqPtrLoc = bankAmt + i + 5;
					printf("Found pointer to sequence table at address 0x%04x!\n", seqPtrLoc);
					seqOffset = ReadLE16(&romData[seqPtrLoc - bankAmt]);
					printf("Sequence table starts at 0x%04x...\n", seqOffset);
					cmdFmt = 2;
					break;

				}
			}

			/*Method 4 - Parasol Stars*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], Table2FindD, 9))
				{
					seqPtrLoc = bankAmt + i + 9;
					printf("Found pointer to sequence table at address 0x%04x!\n", seqPtrLoc);
					seqOffset = ReadLE16(&romData[seqPtrLoc - bankAmt]);
					printf("Sequence table starts at 0x%04x...\n", seqOffset);
					cmdFmt = 3;
					break;

				}
			}

			/*Method 5 - Super James Pond*/
			for (i = 0; i < bankSize; i++)
			{
				if (!memcmp(&romData[i], Table2FindE, 7))
				{
					seqPtrLoc = bankAmt + i + 11;
					printf("Found pointer to sequence table at address 0x%04x!\n", seqPtrLoc);
					seqOffset = romData[seqPtrLoc - bankAmt] + (romData[seqPtrLoc + 3 - bankAmt] * 0x100);
					printf("Sequence table starts at 0x%04x...\n", seqOffset);
					cmdFmt = 1;
					break;

				}
			}

			if (tableOffset != NULL)
			{
				songNum = 1;
				i = tableOffset;
				if (format == 1)
				{
					while (i != seqOffset && ReadLE16(&romData[i - bankAmt]) > bankSize)
					{
						seqPtrs[0] = ReadLE16(&romData[i - bankAmt]);
						printf("Song %i channel 1: 0x%04x\n", songNum, seqPtrs[0]);
						seqPtrs[1] = ReadLE16(&romData[i + 2 - bankAmt]);
						printf("Song %i channel 2: 0x%04x\n", songNum, seqPtrs[1]);
						seqPtrs[2] = ReadLE16(&romData[i + 4 - bankAmt]);
						printf("Song %i channel 3: 0x%04x\n", songNum, seqPtrs[2]);
						seqPtrs[3] = ReadLE16(&romData[i + 6 - bankAmt]);
						printf("Song %i channel 4: 0x%04x\n", songNum, seqPtrs[3]);
						song2txt(songNum, seqPtrs);
						songNum++;
						i += 8;
					}
				}
				else if (format == 2)
				{
					while (i != seqOffset && ReadLE16(&romData[i - bankAmt]) > bankSize)
					{
						seqPtrs[0] = ReadLE16(&romData[i - bankAmt]);
						printf("Song %i channel 1: 0x%04x\n", songNum, seqPtrs[0]);
						seqPtrs[1] = ReadLE16(&romData[i + 2 - bankAmt]);
						printf("Song %i channel 2: 0x%04x\n", songNum, seqPtrs[1]);
						seqPtrs[2] = ReadLE16(&romData[i + 4 - bankAmt]);
						printf("Song %i channel 3: 0x%04x\n", songNum, seqPtrs[2]);
						seqPtrs[3] = ReadLE16(&romData[i + 6 - bankAmt]);
						printf("Song %i channel 4: 0x%04x\n", songNum, seqPtrs[3]);
						song2txt(songNum, seqPtrs);
						songNum++;
						i += 9;
					}
				}

				else if (format == 4)
				{
					i = tableOffset;
					songNum = 1;
					while (ReadLE16(&romData[i + 1 - bankAmt]) != 0)
					{
						songTempo = romData[i - bankAmt];
						printf("Song %i tempo: %i\n", songNum, songTempo);
						songPtr = ReadLE16(&romData[i + 1 - bankAmt]);
						printf("Song %i location: 0x%04X\n", songNum, songPtr);
						nutzSong2txt(songNum, songPtr, songTempo);
						songNum++;
						i += 3;
					}
				}

				if (format != 4)
				{
					getSeqList(seqList, seqOffset);
				}

				seqs2txt(seqList);

			}
			printf("The operation was successfully completed!\n");
		}
	}
}

/*Convert the song data to TXT*/
void song2txt(int songNum, long ptrs[4])
{
	long curPos = 0;
	int index = 0;
	int curSeq = 0;
	int curChan = 0;
	long pattern[4];
	signed int transpose = 0;
	int endSong = 0;
	int jumpPos = 0;


	sprintf(outfile, "song%d.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.txt!\n", songNum);
		exit(2);
	}
	else
	{
		if (cmdFmt == 1)
		{
			for (curChan = 0; curChan < 4; curChan++)
			{
				endSong = 0;
				fprintf(txt, "Channel %i:\n", curChan + 1);
				curPos = ptrs[curChan] - bankAmt;
				while (endSong == 0)
				{
					pattern[0] = romData[curPos];
					pattern[1] = romData[curPos + 1];
					pattern[2] = romData[curPos + 2];
					pattern[3] = romData[curPos + 3];

					if (pattern[0] == 0x80)
					{
						fprintf(txt, "Loop: %i times\n", pattern[1]);
						curPos += 2;
					}
					else if (pattern[0] == 0x82)
					{
						fprintf(txt, "Transpose sequence: %i\n", (signed char)pattern[1]);
						curPos += 2;
					}
					else if (pattern[0] == 0x84)
					{
						fprintf(txt, "Jump to position: 0x%04X\n", ReadLE16(&romData[curPos + 1]));
						endSong = 1;
					}
					else if (pattern[0] == 0x86)
					{
						fprintf(txt, "End of channel\n");
						endSong = 1;
					}
					else if (pattern[0] == 0x88)
					{
						fprintf(txt, "Exit\n");
						endSong = 1;
					}
					else if (pattern[0] == 0xB0)
					{
						fprintf(txt, "Transpose song: %i\n", (signed char)pattern[1]);
						curPos+=2;
					}
					else if (pattern[0] < 0x80)
					{
						fprintf(txt, "Play pattern: %i\n", pattern[0]);
						curPos++;
					}
				}
				fprintf(txt, "\n");
			}
		}
		else if (cmdFmt == 2)
		{
			for (curChan = 0; curChan < 4; curChan++)
			{
				endSong = 0;
				fprintf(txt, "Channel %i:\n", curChan + 1);
				curPos = ptrs[curChan] - bankAmt;
				while (endSong == 0)
				{
					pattern[0] = romData[curPos];
					pattern[1] = romData[curPos + 1];
					pattern[2] = romData[curPos + 2];
					pattern[3] = romData[curPos + 3];

					if (pattern[0] == 0xC0)
					{
						fprintf(txt, "Loop: %i times\n", pattern[1]);
						curPos += 2;
					}
					else if (pattern[0] == 0xC2)
					{
						fprintf(txt, "Transpose sequence: %i\n", (signed char)pattern[1]);
						curPos += 2;
					}
					else if (pattern[0] == 0xC4)
					{
						fprintf(txt, "Jump to position: 0x%04X\n", ReadLE16(&romData[curPos + 1]));
						endSong = 1;
					}
					else if (pattern[0] == 0xC6)
					{
						fprintf(txt, "End of channel\n");
						endSong = 1;
					}
					else if (pattern[0] == 0xC8)
					{
						fprintf(txt, "Exit\n");
						endSong = 1;
					}
					else if (pattern[0] < 0xC0)
					{
						fprintf(txt, "Play pattern: %i\n", pattern[0]);
						curPos++;
					}
				}
				fprintf(txt, "\n");
			}
		}

		else if (cmdFmt == 3)
		{
			for (curChan = 0; curChan < 4; curChan++)
			{
				endSong = 0;
				fprintf(txt, "Channel %i:\n", curChan + 1);
				curPos = ptrs[curChan] - bankAmt;
				while (endSong == 0)
				{
					pattern[0] = romData[curPos];
					pattern[1] = romData[curPos + 1];
					pattern[2] = romData[curPos + 2];
					pattern[3] = romData[curPos + 3];

					if (pattern[0] == 0xEA)
					{
						fprintf(txt, "Jump to position: 0x%04X\n", ReadLE16(&romData[curPos + 1]));
						endSong = 1;
					}
					else if (pattern[0] == 0xE9)
					{
						fprintf(txt, "Transpose sequence: %i\n", (signed char)pattern[1]);
						curPos += 2;
					}
					else if (pattern[0] == 0xE8)
					{
						fprintf(txt, "Loop: %i times\n", pattern[1]);
						curPos += 2;
					}
					else if (pattern[0] == 0xEB)
					{
						fprintf(txt, "Exit\n");
						endSong = 1;
					}
					else if (pattern[0] == 0xEF)
					{
						fprintf(txt, "Set duty: %i\n", pattern[1]);
						curPos += 2;
					}
					else if (pattern[0] == 0xCB)
					{
						fprintf(txt, "Unknown jump: 0x%04X\n", ReadLE16(&romData[curPos + 1]));
						endSong = 1;
					}
					else if (pattern[0] < 0x90)
					{
						fprintf(txt, "Play pattern: %i\n", pattern[0]);
						curPos++;
					}
					else if (pattern[0] == 0x89)
					{
						endSong = 1;
					}
				}
				fprintf(txt, "\n");
			}
		}

	}

}


/*Special method for Mr. Nutz*/
void nutzSong2txt(int songNum, long ptr, int tempo)
{
	int curPos = 0;
	long patPtr = 0;
	long seqPtrs[4];
	int patNum = 0;
	int curTrack = 0;
	long curSeq = 0;

	curPos = ptr - bankAmt;
	patNum = 1;

	sprintf(outfile, "song%d.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%d.txt!\n", songNum);
		exit(2);
	}
	else
	{
		while (ReadLE16(&romData[curPos]) != 0xFFFF && ReadLE16(&romData[curPos]) != 0x0000)
		{
			fprintf(txt, "Pattern %i:\n", patNum);
			patPtr = ReadLE16(&romData[curPos]) - bankAmt;
			for (curTrack = 0; curTrack < 4; curTrack++)
			{
				seqPtrs[curTrack] = ReadLE16(&romData[patPtr + (curTrack * 2)]);
				fprintf(txt, "Channel %i sequence: 0x%04X\n", (curTrack + 1), seqPtrs[curTrack]);
				curSeq = seqPtrs[curTrack];

				for (j = 0; j < 500; j++)
				{
					if (seqList[j] == curSeq)
					{
						break;
					}
				}
				if (j == 500)
				{
					seqList[highestSeq] = curSeq;
					highestSeq++;
				}

			}
			fprintf(txt, "\n");
			curPos += 2;
		}

		if (ReadLE16(&romData[curPos]) == 0xFFFF)
		{
			fprintf(txt, "Loop point: 0x%04X\n", ReadLE16(&romData[curPos + 2]));
		}

		else if (ReadLE16(&romData[curPos]) == 0x0000)
		{
			fprintf(txt, "Track end, no loop point\n");
		}

		fclose(txt);
	}

}


void getSeqList(unsigned long list[], long offset)
{
	int cnt = 0;
	unsigned long curValue;
	unsigned long curValue2;
	long newOffset = offset;
	long offset2 = offset - bankAmt;

	for (cnt = 0; cnt < 500; cnt++)
	{
		curValue = (ReadLE16(&romData[newOffset - bankAmt])) - bankAmt;
		curValue2 = (ReadLE16(&romData[newOffset - bankAmt]));
		if (curValue2 >= bankAmt && curValue2 < (bankAmt * 2))
		{
			list[cnt] = curValue;
			newOffset += 2;
		}
		else
		{
			totalSeqs = cnt;
			break;
		}
	}
}

void seqs2txt(unsigned long list[])
{
	int songEnd = 0;
	sprintf(outfile, "seqs.txt");
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file seqs.txt!\n");
		exit(2);
	}
	else
	{
		unsigned char command[4];
		int seqNum = 0;
		int curPos = 0;
		int nextPos = 0;
		int curNote = 0;
		int k = 0;
		int seqsLeft = totalSeqs;
		long jumpAdr = 0;
		int seqEnd = 0;
		int automatic = 1;
		int curNoteLen = 0;
		int songTrans = 0;
		curPos = list[0];
		if (cmdFmt == 1)
		{
			for (k = 0; k < totalSeqs; k++)
			{
				fprintf(txt, "Sequence %i:\n", k);
				seqEnd = 0;
				curPos = seqList[k];
				automatic = 1;
				while (seqEnd == 0)
				{
					command[0] = romData[curPos];
					command[1] = romData[curPos + 1];
					command[2] = romData[curPos + 2];
					command[3] = romData[curPos + 3];

					if (command[0] == 0x84)
					{
						jumpAdr = ReadLE16(&romData[curPos + 1]);
						fprintf(txt, "Jump to address: 0x%04X\n", jumpAdr);
						seqEnd = 1;
					}

					else if (command[0] == 0x80)
					{
						fprintf(txt, "Loop: %i times\n", command[1]);
						curPos += 2;
					}
					else if (command[0] == 0x82)
					{
						fprintf(txt, "Transpose sequence: %i\n", (signed char)command[1]);
						curPos += 2;
					}

					else if (command[0] == 0x86)
					{
						fprintf(txt, "End sequence\n");
						seqEnd = 1;
					}

					else if (command[0] == 0x88)
					{
						automatic = 1;
						curNoteLen = command[1];
						fprintf(txt, "Set automatic note length: %i\n", curNoteLen);
						curPos += 2;
					}

					else if (command[0] == 0x8A)
					{
						automatic = 0;
						fprintf(txt, "Set manual note length\n");
						curPos++;
					}

					else if (command[0] == 0x8C)
					{
						fprintf(txt, "'Hold' note by 256\n");
						curPos++;
					}

					else if (command[0] == 0x8E)
					{
						if (automatic == 0)
						{
							fprintf(txt, "Rest: %i\n", command[1]);
							curPos += 2;
						}
						else if (automatic == 1)
						{
							fprintf(txt, "Rest\n");
							curPos++;
						}

					}

					else if (command[0] == 0x90)
					{
						fprintf(txt, "Turn on portamento: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0x92)
					{
						fprintf(txt, "Turn off portamento\n");
						curPos++;
					}

					else if (command[0] == 0x94)
					{
						fprintf(txt, "Turn on sweep: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0x96)
					{
						fprintf(txt, "Turn off sweep effect\n");
						curPos++;
					}

					else if (command[0] == 0x98)
					{
						fprintf(txt, "Turn on vibrato: %i %i %i \n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] == 0x9A)
					{
						fprintf(txt, "Turn off vibrato\n");
						curPos++;
					}

					else if (command[0] == 0x9C)
					{
						fprintf(txt, "Turn on arpeggio: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0x9E)
					{
						fprintf(txt, "Turn off arpeggio\n");
						curPos++;
					}

					else if (command[0] == 0xA0)
					{
						fprintf(txt, "Set envelope: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xA2)
					{
						fprintf(txt, "Play drum: %i\n", command[1]);
						curPos++;
					}

					else if (command[0] == 0xA4)
					{
						fprintf(txt, "Poke location: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0xA6)
					{
						fprintf(txt, "Switch waveform\n");
						curPos += 17;
					}

					else if (command[0] == 0xA8)
					{
						fprintf(txt, "Switch waveform\n");
						curPos += 17;
					}

					else if (command[0] == 0xAA)
					{
						fprintf(txt, "Switch waveform from stored address\n");
						curPos++;
					}

					else if (command[0] == 0xAC)
					{
						fprintf(txt, "Set duty: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xAE)
					{
						fprintf(txt, "Set manual sweep: %i, %i, %i\n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] == 0xB0)
					{
						fprintf(txt, "Set registers directly: %i, %i, %i\n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] < 0x80)
					{
						if (command[1] >= 0x80 && command[1] <= 0xB0)
						{
							automatic = 1;
						}
						if (automatic == 0)
						{
							curNote = command[0];
							curNoteLen = command[1];
							fprintf(txt, "Play note: %i, length %i\n", curNote, curNoteLen);
							curPos += 2;
						}
						else if (automatic == 1)
						{
							curNote = command[0];
							fprintf(txt, "Play note: %i\n", curNote);
							curPos++;
						}

					}

					else if (command[0] >= 0xC0)
					{
						curNoteLen = command[0] - 0xC0;
						fprintf(txt, "Set note length: %i\n", curNoteLen);
						curPos++;
					}

					else if (command[0] == 0xB2)
					{
						fprintf(txt, "Set note size: %i\n", command[1]);
						curPos += 2;
					}

					else
					{
						fprintf(txt, "Unknown command %i\n", command[0]);
						curPos++;
					}
				}
				seqsLeft--;
				fprintf(txt, "\n");
			}
			}
		else if (cmdFmt == 2)
		{
			for (k = 0; k < totalSeqs; k++)
			{
				fprintf(txt, "Sequence %i:\n", k);
				seqEnd = 0;
				curPos = seqList[k];
				automatic = 1;
				while (seqEnd == 0)
				{
					command[0] = romData[curPos];
					command[1] = romData[curPos + 1];
					command[2] = romData[curPos + 2];
					command[3] = romData[curPos + 3];
					if (seqDiff == 1 && command[0] > 0xC6)
					{
						command[0] += 2;
					}

					if (command[0] == 0xC4)
					{
						jumpAdr = ReadLE16(&romData[curPos + 1]);
						fprintf(txt, "Jump to address: 0x%04X\n", jumpAdr);
						seqEnd = 1;
					}

					else if (command[0] == 0xC2)
					{
						fprintf(txt, "Transpose sequence: %i\n", (signed char)command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xC6)
					{
						fprintf(txt, "End sequence\n");
						seqEnd = 1;
					}

					else if (command[0] == 0xC8)
					{
						fprintf(txt, "Exit\n");
						seqEnd = 1;
					}

					else if (command[0] == 0xCA)
					{
						automatic = 1;
						curNoteLen = command[1];
						fprintf(txt, "Set automatic note length: %i\n", curNoteLen);
						curPos += 2;
					}

					else if (command[0] == 0xCC)
					{
						automatic = 0;
						fprintf(txt, "Set manual note length\n");
						curPos++;
					}

					else if (command[0] == 0xCE)
					{
						fprintf(txt, "'Hold' note by 256\n");
						curPos++;
					}

					else if (command[0] == 0xD0)
					{
						if (automatic == 0)
						{
							fprintf(txt, "Rest: %i\n", command[1]);
							curPos+=2;
						}
						else if (automatic == 1)
						{
							fprintf(txt, "Rest\n");
							curPos++;
						}

					}

					else if (command[0] == 0xD2)
					{
						fprintf(txt, "Turn on portamento: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0xD4)
					{
						fprintf(txt, "Turn off portamento\n");
						curPos++;
					}

					else if (command[0] == 0xD6)
					{
						fprintf(txt, "Turn on sweep: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0xD8)
					{
						fprintf(txt, "Turn off sweep effect\n");
						curPos++;
					}

					else if (command[0] == 0xDA)
					{
						fprintf(txt, "Turn on vibrato: %i %i %i \n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] == 0xDC)
					{
						fprintf(txt, "Turn off vibrato\n");
						curPos++;
					}

					else if (command[0] == 0xDE)
					{
						fprintf(txt, "Turn on arpeggio: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xE0)
					{
						fprintf(txt, "Turn off arpeggio\n");
						curPos++;
					}

					else if (command[0] == 0xE2)
					{
						fprintf(txt, "Set envelope: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xE4)
					{
						fprintf(txt, "Play drum: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xE6)
					{
						fprintf(txt, "Poke location: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0xE8)
					{
						fprintf(txt, "Switch waveform\n");
						curPos += 17;
					}

					else if (command[0] == 0xEA)
					{
						fprintf(txt, "Switch waveform\n");
						curPos += 17;
					}

					else if (command[0] == 0xEC)
					{
						fprintf(txt, "Switch waveform from stored address\n");
						curPos++;
					}

					else if (command[0] == 0xEE)
					{
						fprintf(txt, "Set duty: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xF0)
					{
						fprintf(txt, "Set manual sweep: %i, %i, %i\n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] == 0xF2)
					{
						fprintf(txt, "Set registers directly: %i, %i, %i\n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] < 0xC0)
					{
						if (automatic == 0)
						{
							curNote = command[0];
							curNoteLen = command[1];
							fprintf(txt, "Play note: %i, length %i\n", curNote, curNoteLen);
							curPos += 2;
						}
						else if (automatic == 1)
						{
							curNote = command[0];
							fprintf(txt, "Play note: %i\n", curNote);
							curPos++;
						}
					}

					else
					{
						fprintf(txt, "Unknown command %i\n", command[0]);
						curPos++;
					}
				}
				seqsLeft--;
				fprintf(txt, "\n");
			}
		}
		else if (cmdFmt == 3)
		{
			for (k = 0; k < totalSeqs; k++)
			{
				fprintf(txt, "Sequence %i:\n", k);
				seqEnd = 0;
				curPos = seqList[k];
				automatic = 1;
				while (seqEnd == 0)
				{
					command[0] = romData[curPos];
					command[1] = romData[curPos + 1];
					command[2] = romData[curPos + 2];
					command[3] = romData[curPos + 3];

					if (command[0] == 0xE8)
					{
						jumpAdr = ReadLE16(&romData[curPos + 1]);
						fprintf(txt, "Jump to address: 0x%04X\n", jumpAdr);
						seqEnd = 1;
					}

					else if (command[0] == 0xE9)
					{
						fprintf(txt, "Transpose sequence: %i\n", (signed char)command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xEA)
					{
						fprintf(txt, "End sequence\n");
						seqEnd = 1;
					}

					else if (command[0] == 0xEB)
					{
						fprintf(txt, "Exit\n");
						seqEnd = 1;
					}

					else if (command[0] == 0xEC)
					{
						automatic = 1;
						curNoteLen = command[1];
						fprintf(txt, "Set automatic note length: %i\n", curNoteLen);
						curPos += 2;
					}

					else if (command[0] == 0xED)
					{
						automatic = 0;
						fprintf(txt, "Set manual note length\n");
						curPos++;
					}

					else if (command[0] == 0xEE)
					{
						fprintf(txt, "'Hold' note by 256\n");
						curPos++;
					}

					else if (command[0] == 0xEF)
					{
						if (automatic == 0)
						{
							fprintf(txt, "Rest: %i\n", command[1]);
							curPos += 2;
						}
						else if (automatic == 1)
						{
							fprintf(txt, "Rest\n");
							curPos++;
						}

					}

					else if (command[0] == 0xF0)
					{
						fprintf(txt, "Turn on portamento: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0xF1)
					{
						fprintf(txt, "Turn off portamento\n");
						curPos++;
					}

					else if (command[0] == 0xF2)
					{
						fprintf(txt, "Turn on sweep: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0xF3)
					{
						fprintf(txt, "Turn off sweep effect\n");
						curPos++;
					}

					else if (command[0] == 0xF4)
					{
						fprintf(txt, "Turn on vibrato: %i %i %i \n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] == 0xF5)
					{
						fprintf(txt, "Turn off vibrato\n");
						curPos++;
					}

					else if (command[0] == 0xF6)
					{
						fprintf(txt, "Turn on arpeggio: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xF7)
					{
						fprintf(txt, "Turn off arpeggio\n");
						curPos++;
					}

					else if (command[0] == 0xF8)
					{
						fprintf(txt, "Set envelope: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xF9)
					{
						fprintf(txt, "Play drum: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xFA)
					{
						fprintf(txt, "Poke location: %i %i\n", command[1], command[2]);
						curPos += 3;
					}

					else if (command[0] == 0xFB)
					{
						fprintf(txt, "Switch waveform\n");
						curPos += 17;
					}

					else if (command[0] == 0xFC)
					{
						fprintf(txt, "Switch waveform\n");
						curPos += 17;
					}

					else if (command[0] == 0xFD)
					{
						fprintf(txt, "Switch waveform from stored address\n");
						curPos++;
					}

					else if (command[0] == 0xFE)
					{
						fprintf(txt, "Set duty: %i\n", command[1]);
						curPos += 2;
					}

					else if (command[0] == 0xFF)
					{
						fprintf(txt, "Set manual sweep: %i, %i, %i\n", command[1], command[2], command[3]);
						curPos += 4;
					}

					else if (command[0] < 0xE8)
					{
						if (automatic == 0)
						{
							curNote = command[0];
							curNoteLen = command[1];
							fprintf(txt, "Play note: %i, length %i\n", curNote, curNoteLen);
							curPos += 2;
						}
						else if (automatic == 1)
						{
							curNote = command[0];
							fprintf(txt, "Play note: %i\n", curNote);
							curPos++;
						}
					}

					else
					{
						fprintf(txt, "Unknown command %i\n", command[0]);
						curPos++;
					}
				}
				seqsLeft--;
				fprintf(txt, "\n");
			}

		}

	else if (cmdFmt == 4)
	{
		for (k = 0; k < highestSeq; k++)
		{
			fprintf(txt, "Sequence %04X:\n", seqList[k]);
			seqEnd = 0;
			curPos = seqList[k] - bankAmt;
			automatic = 1;
			while (seqEnd == 0)
			{
				command[0] = romData[curPos];
				command[1] = romData[curPos + 1];
				command[2] = romData[curPos + 2];
				command[3] = romData[curPos + 3];

				if (command[0] == 0x81)
				{
					fprintf(txt, "Transpose sequence: %i\n", (signed char)command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x82)
				{
					fprintf(txt, "End sequence\n");
					seqEnd = 1;
				}

				else if (command[0] == 0x83)
				{
					automatic = 1;
					curNoteLen = command[1];
					fprintf(txt, "Set automatic note length: %i\n", curNoteLen);
					curPos += 2;
				}

				else if (command[0] == 0x84)
				{
					automatic = 0;
					fprintf(txt, "Set manual note length\n");
					curPos++;
				}

				else if (command[0] == 0x85)
				{
					fprintf(txt, "'Hold' note by 256\n");
					curPos++;
				}

				else if (command[0] == 0x86)
				{
					if (automatic == 0)
					{
						fprintf(txt, "Rest: %i\n", command[1]);
						curPos += 2;
					}
					else if (automatic == 1)
					{
						fprintf(txt, "Rest\n");
						curPos++;
					}

				}

				else if (command[0] == 0x87)
				{
					fprintf(txt, "Turn on portamento: %i %i\n", command[1], command[2]);
					curPos += 3;
				}

				else if (command[0] == 0x88)
				{
					fprintf(txt, "Turn off portamento\n");
					curPos++;
				}

				else if (command[0] == 0x89)
				{
					fprintf(txt, "Turn on sweep: %i %i\n", command[1], command[2]);
					curPos += 3;
				}

				else if (command[0] == 0x8A)
				{
					fprintf(txt, "Turn off sweep effect\n");
					curPos++;
				}

				else if (command[0] == 0x8B)
				{
					fprintf(txt, "Turn on vibrato: %i %i %i \n", command[1], command[2], command[3]);
					curPos += 4;
				}

				else if (command[0] == 0x8C)
				{
					fprintf(txt, "Turn off vibrato\n");
					curPos++;
				}

				else if (command[0] == 0x8D)
				{
					fprintf(txt, "Turn on arpeggio: %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x8E)
				{
					fprintf(txt, "Turn off arpeggio\n");
					curPos++;
				}

				else if (command[0] == 0x8F)
				{
					fprintf(txt, "Set envelope: %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x90)
				{
					fprintf(txt, "Play drum: %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x91)
				{
					fprintf(txt, "Poke location: %i %i\n", command[1], command[2]);
					curPos += 3;
				}

				else if (command[0] == 0x92)
				{
					fprintf(txt, "Switch waveform\n");
					curPos += 17;
				}

				else if (command[0] == 0x93)
				{
					fprintf(txt, "Switch waveform\n");
					curPos += 17;
				}

				else if (command[0] == 0x94)
				{
					fprintf(txt, "Switch waveform from stored address\n");
					curPos++;
				}

				else if (command[0] == 0x95)
				{
					fprintf(txt, "Set duty: %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x96)
				{
					fprintf(txt, "Set manual sweep: %i, %i, %i\n", command[1], command[2], command[3]);
					curPos += 4;
				}

				else if (command[0] == 0x97)
				{
					songTrans = (signed char)command[1];
					fprintf(txt, "Transpose song: %i\n", songTrans);
					curPos += 2;
				}

				else if (command[0] == 0x98)
				{
					fprintf(txt, "Set envelope (v2): %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x99)
				{
					fprintf(txt, "Set volume effect?: %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] >= 0x9A && command[0] <= 0x9D)
				{
					fprintf(txt, "Set duty (v2): %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x9E)
				{
					fprintf(txt, "Change speed?: %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] == 0x9F)
				{
					fprintf(txt, "Change tempo: %i\n", command[1]);
					curPos += 2;
				}

				else if (command[0] >= 0xC0)
				{
					fprintf(txt, "Set note length: %i\n", command[0] - 0xC0);
					curPos++;
				}

				else if (command[0] < 0x80)
				{
					if (automatic == 0)
					{
						curNote = command[0];
						curNoteLen = command[1];
						fprintf(txt, "Play note: %i, length %i\n", curNote, curNoteLen);
						curPos += 2;
					}
					else if (automatic == 1)
					{
						curNote = command[0];
						fprintf(txt, "Play note: %i\n", curNote);
						curPos++;
					}
				}

				else
				{
					fprintf(txt, "Unknown command %i\n", command[0]);
					curPos++;
				}
			}
			seqsLeft--;
			fprintf(txt, "\n");
		}

		}



		fclose(txt);

	}
}