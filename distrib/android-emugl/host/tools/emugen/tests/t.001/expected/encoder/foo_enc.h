// Generated Code - DO NOT EDIT !!
// generated by 'emugen'

#ifndef GUARD_foo_encoder_context_t
#define GUARD_foo_encoder_context_t

#include "IOStream.h"
#include "foo_client_context.h"


#include "fooUtils.h"
#include "fooBase.h"

struct foo_encoder_context_t : public foo_client_context_t {

	IOStream *m_stream;

	foo_encoder_context_t(IOStream *stream);
};

#endif  // GUARD_foo_encoder_context_t
