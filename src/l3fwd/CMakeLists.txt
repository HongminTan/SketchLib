cmake_minimum_required(VERSION 3.16)

set(SRCS
	main.c
	l3fwd_acl.c
	l3fwd_lpm.c
	l3fwd_fib.c
	l3fwd_em.c
	l3fwd_event.c
	l3fwd_event_generic.c
	l3fwd_event_internal_port.c
)

add_executable(l3fwd-shared ${SRCS})
target_link_libraries(l3fwd-shared PRIVATE PkgConfig::DPDK)
set_target_properties(l3fwd-shared PROPERTIES OUTPUT_NAME "l3fwd")
