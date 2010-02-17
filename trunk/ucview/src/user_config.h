/*
** user_config.h
** 
*/

#include <unicap.h>

#ifndef   	USER_CONFIG_H_
# define   	USER_CONFIG_H_

unicap_handle_t user_config_try_restore( void );
void user_config_store_default( unicap_handle_t handle );
void user_config_store_device( unicap_handle_t handle );
void user_config_restore_device( unicap_handle_t handle );

#endif 	    /* !USER_CONFIG_H_ */
