#include <stdio.h>
#include <stdlib.h>




// printing hexdump for wifi scans
#define PRINT_INFO 1





// array of ASCII chars, where their index in the array corresponds to its decimal representation.
// chars that aren't needed for our purposes are left as underscores
const char ASCII_DICTIONARY[] = {'0', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '_', '-', '.', '_', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '_', '_', '_', '_', '_', '_', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'};
// array of hex chars, where their index in the array corresponds to its decimal representation
const char* const HEX_DICTIONARY[] = {"00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F", "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F", "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F", "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F", "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F", "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F", "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF", "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF", "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF", "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF", "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF", "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF"};


typedef struct {
    uint8_t basic_id_flag;
    uint8_t location_vector_flag;
    uint8_t authentication_flag;
    uint8_t self_id_flag;
    uint8_t system_flag;
    uint8_t operator_id_flag;
} msg_flags_t;


void parse_hex(uint8_t* data, int len, int odid_identifier_idx, uint8_t num_msg_in_pack, msg_flags_t* msg_flags) {
    /*
	 @brief: the hex-string (from either wifi scan or bluetooth scan) into RID data fields

     @param[in]  data: buffer of uint8_t that was directly populated from the wifi or bluetooth scans
     @param[in]  len: length of data
     @param[in]  odid_identifier_idx: index in the data buffer of '0D'
     @param[in]  num_msg_in_pack: number of messages in the message pack (max of 9)
     @param[in]  msg_flags: ptr to struct of flags; after this function is run, the flags will be updated to reflect which messages were in the message packs
	 */

    for (int msg_num=0; msg_num<num_msg_in_pack; msg_num++) {
        int msg_type = (data[odid_identifier_idx + 8 + msg_num*25] - 2) / 16;  // either 0 (Basic ID), 1 (Location/Vector), 2 (Authentication), 3 (Self-ID), 4 (System), or 5 (Operator ID)
        
        switch(msg_type) {  // Decode the raw data into useful RID information
            case 0:  // Basic ID Message
                enum UA_TYPE ua_type = data[odid_identifier_idx + 8 + msg_num*25 + 1] % 16;
                enum ID_TYPE id_type = data[odid_identifier_idx + 8 + msg_num*25 + 1] / 16;  // floor division
                if (PRINT_INFO) {
                    printf("ID TYPE: %s.  ", ID_TYPE_STRING[id_type]);
                    printf("UA TYPE: %s.  ", UA_TYPE_STRING[ua_type]);
                }
                
                switch(id_type) {
                    // Using curly bracs around each case to define each case as it's own frame to prevent redeclaration errors for id_buf
                    case 0:{  // None --> null ID
                        char id_buf[] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '\0'};
                        if (PRINT_INFO) {
                            printf("NULL UAV ID: %s.\n\n", id_buf);
                        }
                        break;
                    }
                    case 1:  // Serial Number
                    case 2:{  // CAA Registration
                        // Loop through the Serial number in the raw decimal data and build a new string containing the serial number in ASCII
                        char id_buf[21];  // initalize char array to hold the serial number (which is at most 20 ASCII chars)
                        for (int i=0; i<20; i++) {
                            int decimal_val = data[odid_identifier_idx + 8 + msg_num*25 + 2+i];
                            id_buf[i] = ASCII_DICTIONARY[decimal_val];
                        }
                        id_buf[20] = '\0';
                        if (PRINT_INFO) {
                            printf("SERIAL NUMBER/CAA REGISTRATION NUMBER: %s.\n\n", id_buf);
                        }
                        break;
                    }
                    case 3:{  // UTM UUID (should be encoded as a 128-bit UUID (32-char hex string))
                        char id_buf[33];
                        for (int i=0; i<16; i++) {
                            int decimal_val = data[odid_identifier_idx + 8 + msg_num*25 + 2+i];
                            id_buf[2*i] = HEX_DICTIONARY[decimal_val][0];  // 1st hex char in an array of 2 hex chars
                            id_buf[2*i+1] = HEX_DICTIONARY[decimal_val][1];  // 2nd hex char in an array of 2 hex chars
                        }
                        id_buf[32] = '\0';
                        if (PRINT_INFO) {
                            printf("UTM UUID: %s.\n\n", id_buf);
                        }
                        break;
                    }							
                    case 4:{  // Specific Session ID (1st byte is an in betwen 0 and 255, and 19 remaining bytes are alphanumeric code, according to this: https://www.rfc-editor.org/rfc/rfc9153.pdf)
                        char id_buf[21];
                        id_buf[0] = data[odid_identifier_idx + 8 + msg_num*25 + 2];
                        // sprintf(id_buf[0], "%d", data[odid_identifier_idx + 8 + msg_num*25 + 2]);
                        // Loop through the Session ID in the raw decimal data and build a new string containing the serial number in ASCII
                        for (int i=1; i<20; i++) {
                            int decimal_val = data[odid_identifier_idx + 8 + msg_num*25 + 2+i];
                            id_buf[i] = ASCII_DICTIONARY[decimal_val];
                        }
                        id_buf[20] = '\0';
                        if (PRINT_INFO) {
                            printf("SPECIFIC SESSION ID: %s.\n\n", id_buf);
                        }
                        break;
                    }
                }
                msg_flags->basic_id_flag = 1;
                break;
            case 1:  // Location/Vector Message
                enum OPERATIONAL_STATUS op_status = data[odid_identifier_idx + 8 + msg_num*25 + 1] / 16;  // floor division
                // funky modulos & floor division to get the value of each bit
                enum HEIGHT_TYPE height_type_flag = (data[odid_identifier_idx + 8 + msg_num*25 + 1] % 8) / 4;  // 0: Above Takeoff. 1: AGL
                enum E_W_DIRECTION_SEGMENT direction_segment_flag = (data[odid_identifier_idx + 8 + msg_num*25 + 1] % 4) / 2;  // 0: <180, 1: >=180
                enum SPEED_MULTIPLIER speed_multiplier_flag = data[odid_identifier_idx + 8 + msg_num*25 + 1] % 2;;  // 0: x0.25, 1: x0.75

                uint8_t track_direction = data[odid_identifier_idx + 8 + msg_num*25 + 2];  // direction measured clockwise from true north. Should be between 0-179
                if (direction_segment_flag) {  // If E_W_DIRECTION_SEGMENT is true, add 180 to the track_direction, according to ASTM.
                    track_direction += 180;
                }

                uint8_t speed = data[odid_identifier_idx + 8 + msg_num*25 + 3];  // ground speed in m/s
                if (speed_multiplier_flag) {
                    speed = 0.75*speed + 255*0.25;  // as defined in ASTM
                }
                else {
                    speed *= 0.25;
                }

                uint8_t vertical_speed = data[odid_identifier_idx + 8 + msg_num*25 + 4] * 0.5;  // vertical speed in m/s (positive = up, negatve = down); multiply by 0.5 as defined in ASTM

                // lat and lon Little Endian encoded
                int32_t lat_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 5];
                int32_t lat_lsb1 = data[odid_identifier_idx + 8 + msg_num*25 + 6] << 8;  // multiply by 2^8
                int32_t lat_msb1 = data[odid_identifier_idx + 8 + msg_num*25 + 7] << 16;  // multiply by 2^16
                int32_t lat_msb = data[odid_identifier_idx + 8 + msg_num*25 + 8] << 24;  // multiply by 2^24
                int32_t lat_int = lat_msb + lat_msb1 + lat_lsb1 + lat_lsb;
                float lat = (float) lat_int / 10000000.0;  // divide by 10^7, according to ASTM

                int32_t lon_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 9];
                int32_t lon_lsb1 = data[odid_identifier_idx + 8 + msg_num*25 + 10] << 8;  // multiply by 2^8
                int32_t lon_msb1 = data[odid_identifier_idx + 8 + msg_num*25 + 11] << 16;  // multiply by 2^16
                int32_t lon_msb = data[odid_identifier_idx + 8 + msg_num*25 + 12] << 24;  // multiply by 2^24
                int32_t lon_int = lon_msb + lon_msb1 + lon_lsb1 + lon_lsb;
                float lon = (float) lon_int / 10000000.0;  // divide by 10^7, according to ASTM

                // altitudes and height Little Endian encoded
                uint16_t pressure_altitude_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 13];
                uint16_t pressure_altitude_msb = data[odid_identifier_idx + 8 + msg_num*25 + 14] << 8;
                uint16_t pressure_altitude = (pressure_altitude_msb + pressure_altitude_lsb) * 0.5 - 1000;

                uint16_t geodetic_altitude_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 15];
                uint16_t geodetic_altitude_msb = data[odid_identifier_idx + 8 + msg_num*25 + 16] << 8;
                uint16_t geodetic_altitude = (geodetic_altitude_msb + geodetic_altitude_lsb) * 0.5 - 1000;

                uint16_t height_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 17];
                uint16_t height_msb = data[odid_identifier_idx + 8 + msg_num*25 + 18] << 8;
                uint16_t height = (height_msb + height_lsb) * 0.5 - 1000;

                enum VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY vertical_accuracy = data[odid_identifier_idx + 8 + msg_num*25 + 19] / 16;  // floor division
                enum VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY horizontal_accuracy = data[odid_identifier_idx + 8 + msg_num*25 + 19] % 16;
                
                enum VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY baro_alt_accuracy = data[odid_identifier_idx + 8 + msg_num*25 + 20] / 16;  // floor division
                enum SPEED_ACCURACY speed_accuracy = data[odid_identifier_idx + 8 + msg_num*25 + 20] % 16;

                // 1/10ths of seconds since the last hour relatve to UTC time
                uint16_t timestamp_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 21];
                uint16_t timestamp_msb = data[odid_identifier_idx + 8 + msg_num*25 + 21] << 8;
                uint16_t timestamp = timestamp_msb + timestamp_lsb;

                uint8_t timestamp_accuracy_int = (data[odid_identifier_idx + 8 + msg_num*25 + 22] % 15);  // modulo 15 to get rid of bits 7-4. Between 0.1 s and 1.5 s (0 s = unknown)
                float timestamp_accuracy = timestamp_accuracy_int * 0.1;
                
                if (PRINT_INFO) {
                    printf("OPERATIONAL STATUS: %s.  ", OPERATIONAL_STATUS_STRING[op_status]);
                    printf("HEIGHT TYPE: %s.  ", HEIGHT_TYPE_STRING[height_type_flag]);
                    printf("DIRECTION SEGMENT FLAG: %s.  ", E_W_DIRECTION_SEGMENT_STRING[direction_segment_flag]);
                    printf("SPEED MULTIPLIER FLAG: %s.  ", SPEED_MULTIPLIER_STRING[speed_multiplier_flag]);
                    printf("HEADING (deg): %d.  ", track_direction);
                    printf("SPEED (m/s): %d.  ", speed);
                    printf("VERTICAL SPEED (m/s): %d.  ", vertical_speed);
                    printf("LAT: %d.  ", lat_int);
                    printf("LON: %d.  ", lon_int);
                    printf("PRESSURE ALT: %d.  ", pressure_altitude);
                    printf("GEO ALT: %d.  ", geodetic_altitude);
                    printf("HEIGHT: %d.  ", height);
                    printf("HORIZONTAL ACCURACY: %s.  ", VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY_STRING[horizontal_accuracy]);
                    printf("VERTICAL ACCURACY: %s.  ", VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY_STRING[vertical_accuracy]);
                    printf("BARO ALT ACCURACY: %s.  ", VERTICAL_HORIZONTAL_BARO_ALT_ACCURACY_STRING[baro_alt_accuracy]);
                    printf("SPEED ACCURACY: %s.  ", SPEED_ACCURACY_STRING[speed_accuracy]);
                    printf("TIMESTAMP: %d.  ", timestamp);
                    printf("TIMESTAMP_ACCURACY (btwn 0.1-1.5s): %f\n\n", timestamp_accuracy);
                }

                msg_flags->location_vector_flag = 1;
                break;
            case 3:  // Self ID Message
                enum SELF_ID_TYPE self_id_type = data[odid_identifier_idx + 8 + msg_num*25 + 1];
                // Loop through the self id description in the raw decimal data and build a new string containing the self id description in ASCII
                char self_id_description_buf[24];  // initalize char array to hold self id description
                for (int i=0; i<23; i++) {
                    int decimal_val = data[odid_identifier_idx + 8 + msg_num*25 + 2 + i];
                    self_id_description_buf[i] = ASCII_DICTIONARY[decimal_val];
                }
                self_id_description_buf[23] = '\0';
                
                if (PRINT_INFO) {
                    printf("SELF ID TYPE: %s.  ", SELF_ID_TYPE_STRING[self_id_type]);
                    printf("SELF ID: %s.\n\n", self_id_description_buf);
                }
                msg_flags->self_id_flag = 1;
                break;
            case 4:  // System Message
                enum OPERATOR_LOCATION_ALTITUDE_SOURCE_TYPE operator_location_type = data[odid_identifier_idx + 8 + msg_num*25 + 1] % 3; // mod 3 so that we only consider bits 1 and 0
                
                // lat and lon Little Endian encoded
                int32_t operator_lat_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 2];
                int32_t operator_lat_lsb1 = data[odid_identifier_idx + 8 + msg_num*25 + 3] << 8;  // multiply by 2^8
                int32_t operator_lat_msb1 = data[odid_identifier_idx + 8 + msg_num*25 + 4] << 16;  // multiply by 2^16
                int32_t operator_lat_msb = data[odid_identifier_idx + 8 + msg_num*25 + 5] << 24;  // multiply by 2^24
                int32_t operator_lat_int = operator_lat_msb + operator_lat_msb1 + operator_lat_lsb1 + operator_lat_lsb;
                float operator_lat = (float) operator_lat_int / 10000000.0;

                int32_t operator_lon_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 6];
                int32_t operator_lon_lsb1 = data[odid_identifier_idx + 8 + msg_num*25 + 7] << 8;  // multiply by 2^8
                int32_t operator_lon_msb1 = data[odid_identifier_idx + 8 + msg_num*25 + 8] << 16;  // multiply by 2^16
                int32_t operator_lon_msb = data[odid_identifier_idx + 8 + msg_num*25 + 9] << 24;  // multiply by 2^24
                int32_t operator_lon_int = operator_lon_msb + operator_lon_msb1 + operator_lon_lsb1 + operator_lon_lsb;
                float operator_lon = (float) operator_lon_int / 10000000.0;

                // number of aircraft in the area
                uint16_t area_count_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 10];
                uint16_t area_count_msb = data[odid_identifier_idx + 8 + msg_num*25 + 11] << 8;  // multiply by 2^8
                uint16_t area_count = area_count_msb + area_count_lsb;

                // radius of cylindrical area with the group of aircraft
                uint16_t area_radius = data[odid_identifier_idx + 8 + msg_num*25 + 12] * 10;

                // floor and ceiling Little Endian encoded
                uint16_t area_ceiling_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 13];
                uint16_t area_ceiling_msb = data[odid_identifier_idx + 8 + msg_num*25 + 14] << 8;
                uint16_t area_ceiling = (area_ceiling_msb + area_ceiling_lsb) * 0.5 - 1000;

                uint16_t area_floor_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 15];
                uint16_t area_floor_msb = data[odid_identifier_idx + 8 + msg_num*25 + 16] << 8;
                uint16_t area_floor = (area_floor_msb + area_floor_lsb) * 0.5 - 1000;

                enum UA_CATEGORY ua_category = data[odid_identifier_idx + 8 + msg_num*25 + 17] / 16;  // floor division
                enum UA_CLASS ua_classification = data[odid_identifier_idx + 8 + msg_num*25 + 17] % 16;
                

                // operator altitude Little Endian encoded
                uint16_t operator_altitude_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 18];
                uint16_t operator_altitude_msb = data[odid_identifier_idx + 8 + msg_num*25 + 19] << 8;
                uint16_t operator_altitude = (operator_altitude_msb + operator_altitude_lsb) * 0.5 - 1000;

                // current time in seconds since 00:00:00 01/01/2019
                uint32_t system_timestamp_lsb = data[odid_identifier_idx + 8 + msg_num*25 + 20];
                uint32_t system_timestamp_lsb1 = data[odid_identifier_idx + 8 + msg_num*25 + 21] << 8;
                uint32_t system_timestamp_msb1 = data[odid_identifier_idx + 8 + msg_num*25 + 22] << 16;
                uint32_t system_timestamp_msb = data[odid_identifier_idx + 8 + msg_num*25 + 23] << 24;
                uint32_t system_timestamp = system_timestamp_msb + system_timestamp_msb1 + system_timestamp_lsb1 + system_timestamp_lsb;

                if (PRINT_INFO) {
                    printf("OPERATOR LOCATION SOURCE TYPE: %s.  ", OPERATOR_LOCATION_ALTITUDE_SOURCE_TYPE_STRING[operator_location_type]);
                    printf("OPERATOR LAT: %d.  ", operator_lat_int);
                    printf("OPERATOR LON: %d.  ", operator_lon_int);
                    printf("OPERATOR ALT: %d.  ", operator_altitude);
                    printf("AREA COUNT: %d.  ", area_count);
                    printf("AREA RADIUS: %d.  ", area_radius);
                    printf("AREA CEILING: %d.  ", area_ceiling);
                    printf("AREA FLOOR: %d.  ", area_floor);
                    printf("UA CATEGORY: %s.  ", UA_CATEGORY_STRING[ua_category]);
                    printf("UA CLASS: %s.  ", UA_CLASS_STRING[ua_classification]);
                    printf("TIMESTAMP (secs from 00:00:00 01/01/2019): %d.\n\n", system_timestamp);
                }

                msg_flags->system_flag = 1;
                break;
            case 5:  // Operator ID Message
                // Loop through the operator id in the raw decimal data and build a new string containing the operator id in ASCII
                char operator_id_buf[21];  // initalize char array to hold the operator id string (which is at most 20 ASCII chars)
                for (int i=0; i<20; i++) {
                    int decimal_val = data[odid_identifier_idx + 8 + msg_num*25 + 2 + i];
                    operator_id_buf[i] = ASCII_DICTIONARY[decimal_val];
                }
                operator_id_buf[20] = '\0';
                if (PRINT_INFO) {
                    printf("OPERATOR ID (CAA-issued License): %s.\n\n", operator_id_buf);
                }
                msg_flags->operator_id_flag = 1;
                break;
        }
    }
}


void log_hexdump(uint8_t* buf, uint16_t size) {
	/*
	 print the buffer (which should be a scanned wifi packet) as a string of hex chars.
	 */
	for (int i=0; i<size; i++) {
		if (buf[i] < 16) {  // because printing "%x ", for single digit characters omits the first 0
			printk("0");
		}
		printk("%X ", buf[i]);
		k_sleep(K_SECONDS(0.002));  // delay between each character so messages aren't dropped
	}
	printk("\n");
}


int contains(uint8_t big[], int size_b, uint8_t small[], int size_s) {
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