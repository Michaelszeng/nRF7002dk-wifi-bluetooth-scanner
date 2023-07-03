#include <stdio.h>
#include <stdlib.h>


// array of ASCII chars, where their index in the array corresponds to its decimal representation.
// chars that aren't needed for our purposes are left as underscores
static const char ASCII_DICTIONARY[] = {'0', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '-', '.', '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_', '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
// array of hex chars, where their index in the array corresponds to its decimal representation
static const char* const HEX_DICTIONARY[] = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F", "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F", "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF", "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF", "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"};


static void parse_hex() {
    /*
	 turn the hex-string (from either wifi scan or bluetooth scan) into RID data fields
	 */
}


static void log_hexdump(uint8_t* buf, uint16_t size) {
	/*
	 print the buffer (which should be a scanned wifi packet) as a string of hex chars.
	 */
	for (int i=0; i<size; i++) {
		if (buf[i] < 16) {  // because printing "%x ", for single digit characters omits the first 0
			printk("0");
		}
		printk("%X ", buf[i]);
		k_sleep(K_SECONDS(0.001));  // delay between each character so messages aren't dropped
	}
	printk("\n");
}


static int contains(uint8_t big[], int size_b, uint8_t small[], int size_s) {
	/*
	 Checks if a small array is a sub-array of a big array. Returns -1 if it's not,
	 Returns the index of the small array in the big array if it is.
	 */
    int contains, k;
    for(int i = 0; i < size_b; i++){
        // assume that the big array contains the small array
        contains = 1;
        // check if the element at index i in the big array is the same as the first element in the small array
       if(big[i] == small[0]){
           // if yes, then we start from k = 1, because we already know that the first element in the small array is the same as the element at index i in the big array
           k = 1;
           // we start to check if the next elements in the big array are the same as the elements in the small array
           // (we start from i+1 position because we already know that the element at the position i is the same as the first element in the small array)
           for(int j = i + 1; j < size_b; j++){
               // range for k must be from 1 to size_s-1
               if(k >= size_s - 1) {
                   break;
               }
               // if the element at the position j in the big array is different
               // from the element at the position k in the small array then we
               // flag that we did not find the sequence we were looking for (contains=0)
               if(big[j] != small[k]){
                   contains = 0;
                   break;
               }
               // increment k because we want the next element in the small array
               k++;
           }
           // if contains flag is not 0 that means we found the sequence we were looking
           // for and that sequence starts from index i in the big array
           if(contains) {
               return i;
           }
       }
    }
    return -1;  // if the sequence we were looking for was not found
}