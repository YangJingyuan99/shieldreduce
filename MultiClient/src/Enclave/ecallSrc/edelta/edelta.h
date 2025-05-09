#ifndef _EDELTA_H
#define _EDELTA_H

#include <cstdint>

#include "htable.h"
#include "util.h"

// Attention! input should not be empty! base should not be empty!
int EDeltaEncode( uint8_t* input, uint32_t input_size,
		  				uint8_t* base, uint32_t base_size,
		  				uint8_t* delta, uint32_t *delta_size );	
    
int EDeltaDecode(uint8_t *delta, uint32_t delta_size,
                  uint8_t *base, uint32_t base_size,
                  uint8_t *output, uint32_t *output_size);
            
#endif // _EDELTA_H