#include "example.ecsact.systems.hh"

void example::ExampleIndexedAction::impl(context& ctx) {
}

void example::ExampleSystemFromImports::impl(context& ctx) {
	auto a = ctx.get<pkg::a::ExampleA>();
	auto b = ctx.get<pkg::b::ExampleB>();

	a.a += 1;
	b.b += 1;

	ctx.update(a);
	ctx.update(b);
}

void example::ExampleLazy::impl(context&) {
}

void example::LazyLeap::impl(context&) {
}

void example::ExplicitlyNoLazy::impl(context&) {
}

void example::ExplicitlyNoLazyZero::impl(context&) {
}

void example::ParallelExample::impl(context&) {
}
