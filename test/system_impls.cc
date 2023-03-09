#include "example.ecsact.systems.hh"

void example::ExampleSystemFromImports::impl(context& ctx) {
	auto a = ctx.get<pkg::a::ExampleA>();
	auto b = ctx.get<pkg::b::ExampleB>();

	a.a += 1;
	b.b += 1;

	ctx.update(a);
	ctx.update(b);
}
