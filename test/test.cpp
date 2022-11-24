#include "MDP_Implementation.h"
#include "utilities.h"

int main()
{
	MDP_Implementation mdp_impl(2);
	auto mdp = Dynaplex::Convert(mdp_impl);

	mdp->write();
	return 0;
}
