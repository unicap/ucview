/*
** worker.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Sun Sep  2 21:08:37 2007 Arne Caspari
*/

#ifndef   	WORKER_H_
# define   	WORKER_H_

void *run_worker( void *(*func)(void *), void *arg );

#endif 	    /* !WORKER_H_ */
