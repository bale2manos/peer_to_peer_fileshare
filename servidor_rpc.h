/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#ifndef _SERVIDOR_RPC_H_RPCGEN
#define _SERVIDOR_RPC_H_RPCGEN

#include <rpc/rpc.h>

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif


#define SERVIDOR_RPC 99
#define VERSION_RPC 1

#if defined(__STDC__) || defined(__cplusplus)
#define print_rpc 1
extern  enum clnt_stat print_rpc_1(char *, int *, CLIENT *);
extern  bool_t print_rpc_1_svc(char *, int *, struct svc_req *);
extern int servidor_rpc_1_freeresult (SVCXPRT *, xdrproc_t, caddr_t);

#else /* K&R C */
#define print_rpc 1
extern  enum clnt_stat print_rpc_1();
extern  bool_t print_rpc_1_svc();
extern int servidor_rpc_1_freeresult ();
#endif /* K&R C */

#ifdef __cplusplus
}
#endif

#endif /* !_SERVIDOR_RPC_H_RPCGEN */
