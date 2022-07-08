#include <cstddef>

#include <iostream> 
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

#include <metis.h>
#include <gtest/gtest.h>

#include "CSRList.hpp"
#include "ElementSpace.hpp"
#include "Mesh.hpp"
#include "MeshIO.hpp"




TEST(CSRList, Constructor) {
	std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
	std::vector<std::size_t> indptr{0, 3, 5, 9};
	CSRList list0(x, indptr);
	EXPECT_EQ(list0.num_entities(), 3);
	CSRList list1(std::move(x), std::move(indptr));
	EXPECT_EQ(list1.num_entities(), 3);

	CSRList list2 = list0;
	EXPECT_EQ(list2.num_entities(), 3);
	CSRList list3 = std::move(list0);
	EXPECT_EQ(list3.num_entities(), 3);
}

TEST(CSRList, Iterators) {
	std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
	std::vector<std::size_t> indptr{0, 3, 5, 9};
	CSRList list(x, indptr);
	auto n_x = std::accumulate(
		list.begin(),
		list.end(),
		static_cast<std::size_t>(0),
		[](std::size_t a, decltype(*list.begin())&& b) { 
			return a + b.data().size();
		});
	EXPECT_EQ(n_x, x.size());	

	std::size_t n_y = 0;
	for(const auto& entity : list) {
		n_y += entity.size();
	}
	EXPECT_EQ(n_y, x.size());
}

/*
TEST(MeshData_read, metis) {
	MeshData mesh;
	mesh.read("ien.dat");

	idx_t* vwgt = nullptr;
	idx_t* vsize = nullptr;
	idx_t ncommon = 1;
	idx_t nparts = 4;
	real_t* tpwgts = nullptr;
	idx_t objval = 1;

	idx_t options[METIS_NOPTIONS];
	METIS_SetDefaultOptions(options);
	options[METIS_OPTION_PTYPE]   = METIS_PTYPE_KWAY;
	options[METIS_OPTION_OBJTYPE] = METIS_OBJTYPE_CUT;
	options[METIS_OPTION_CTYPE]   = METIS_CTYPE_SHEM;
	options[METIS_OPTION_IPTYPE]  = METIS_IPTYPE_GROW;
	options[METIS_OPTION_RTYPE]   = -1;
	options[METIS_OPTION_DBGLVL]  = 0;
	options[METIS_OPTION_UFACTOR] = -1;
	options[METIS_OPTION_MINCONN] = 0;
	options[METIS_OPTION_CONTIG]  = 0;
	options[METIS_OPTION_SEED]    = -1;
	options[METIS_OPTION_NITER]   = 10;
	options[METIS_OPTION_NCUTS]   = 1;
	std::vector<idx_t> epart(mesh.ne), npart(mesh.nn);
	auto status = METIS_PartMeshDual(&(mesh.ne), &(mesh.nn), 
		mesh.eptr.data(), mesh.eind.data(), 
		vwgt, vsize, &ncommon, &nparts,
		tpwgts, options, &objval,
		epart.data(), npart.data());
	EXPECT_EQ(status, METIS_OK);
	std::ofstream fe("ien.epart.dat");
	for(int i = 0; i < epart.size(); ++i) {
		fe << epart[i] << "\n";
	}
	std::ofstream fn("ien.npart.dat");
	for(int i = 0; i < npart.size(); ++i) {
		fn << npart[i] << "\n";
	}
}
*/

TEST(MeshIO, Mesh) {
	Mesh<3> mesh;
	//MeshIO::read<3>(mesh, "box.msh");
	MeshIO::read<3>(mesh, "re3700.msh");
	EXPECT_EQ(mesh.nodes().size(), 1234222*mesh.dim());
	EXPECT_EQ(mesh.elements(FiniteElementType::Tetrahedron).second.size(), 7326679);
	
	const auto nodes = mesh.nodes();
	double x[3] = {0.0, 0.0, 0.0};
	int i = 0;
	
	for(int j = 0; i < nodes.size() / 3; ++j) {
		for(int k = 0; k < mesh.dim(); ++k) {
			x[k] += nodes[j * 3 + k];
		}
		i++;
	}

	//EXPECT_NEAR(x[0]/i, 3.7496478658, 1e-10);
	//EXPECT_NEAR(x[1]/i, -0.005098642278, 1e-10);
	//EXPECT_NEAR(x[2]/i, -0.0188287907804, 1e-10);
	EXPECT_NEAR(x[0]/i, 2.336090445539671, 1e-10);
	EXPECT_NEAR(x[1]/i, 0.0006215432516590993, 1e-10);
	EXPECT_NEAR(x[2]/i, 0.00015412152905954366, 1e-10);

	// numbers of elements
	std::size_t ne[] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	ne[0] = mesh.nodes().size() / mesh.dim();
	//ne[2] = 3220;
	//ne[4] = 741065;
	ne[4] = 7326679;
	/*
	const auto& map = mesh.element_map();
	i = 0;
	for(const auto& [key, value] : map) {
		EXPECT_EQ(key, ElementSpace<3>::type_values[i]);
		EXPECT_EQ(value.first.size(), ne[i]);
		EXPECT_EQ(value.second.size(), ne[i]);
		++i;
	}
	*/
}

int main(int argc, char* argv[] ){
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
