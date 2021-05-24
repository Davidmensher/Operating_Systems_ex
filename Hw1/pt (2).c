#include "os.h"

void destroy_mapping(uint32_t, uint32_t);
void update_mapping(uint32_t, uint32_t, uint32_t);
uint32_t* get_first_address(uint32_t, uint32_t);
uint32_t* get_pp_address(uint32_t, uint32_t);
uint32_t page_table_query(uint32_t, uint32_t);

void page_table_update(uint32_t pt, uint32_t vpn, uint32_t ppn){
	if(ppn == NO_MAPPING){
		destroy_mapping(pt, vpn);
	}
	else{
		update_mapping(pt, vpn, ppn);	
	}
	return;		
}

uint32_t* get_first_address(uint32_t pt, uint32_t vpn){
	uint32_t left_20 = pt << 12;
        uint32_t right_12 = vpn >> 10;
        uint32_t first_adress = left_20 + 4*right_12;
        uint32_t* p = (uint32_t*)(phys_to_virt(first_adress));
	return p;	
}

uint32_t* get_pp_address(uint32_t pn, uint32_t vpn){
	uint32_t right_12 = vpn & 0x3ff;
        uint32_t left_20 = pn<<12;
        uint32_t second_adress = left_20 + 4*right_12;
        uint32_t* p = (uint32_t*)(phys_to_virt(second_adress));
	return p;
}

//an helper function to page_table_update
void destroy_mapping(uint32_t pt, uint32_t vpn){
       
	//getting to the first frame
	uint32_t* p = get_first_address(pt, vpn);
	
	//chekc if the corresponding first layer frame exists
	if(((*p) & 0x1) == 0 ){
		return;
	}
	
	//getting to the ppn
	uint32_t pn = (*p)>>12;
	p = get_pp_address(pn,  vpn);
	
	//change the adress to unvalid
	*p = (*p) & 0xfffffff0;
	return;
}

void update_mapping(uint32_t pt, uint32_t vpn, uint32_t ppn){
	//getting the first frame
	uint32_t* p = get_first_address(pt, vpn);

	//check if there is a first layer frame
	uint32_t pn;
	if(((*p) & 0x1) == 0 ){       
		pn = alloc_page_frame();
		*p = (pn<<12) + 0x1;
	}
	else{
		pn = (*p)>>12;	
	}
	
	//getting ppn address
	p = get_pp_address(pn,  vpn);

	//updating the ppn value
	*p = (ppn<<12) | 0x1;

	return;
}

uint32_t page_table_query(uint32_t pt, uint32_t vpn){
	//getting the first frame
	uint32_t* p = get_first_address(pt, vpn);

	//check if there is a first frame
        if(((*p) & 0x1) ==0x0){
                return NO_MAPPING;
        }	
	
	//else, there is first frame, we keep search
	uint32_t pn = (*p)>>12;
	
	p = get_pp_address(pn,  vpn);

	//cheking if there is valid  translation
	if(((*p) & 0x1) ==0x0){
		return NO_MAPPING;
	}
	
	//there is translation, we return the ppn
	return (*p)>>12 ;

}



