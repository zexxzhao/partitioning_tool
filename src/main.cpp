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

	CSRList list4(list3.begin() + 1, list3.begin() + 2);
	EXPECT_EQ(list4.num_entities(), 1);
}

TEST(CSRList, operator) {
	std::vector<double> x{0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0};
	std::vector<std::size_t> indptr{0, 3, 5, 9};
	CSRList list0(x, indptr);
	CSRList list1(list0.begin(), list0.begin() + 1);
	list0 += list1; // 4: list0.data() == {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 0.0, 1.0, 2.0}
	auto list2 = list0 + list1; // 5
	auto list3 = list1 + list0; // 5: 
	//list3.data() == {0.0, 1.0, 2.0, 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 0.0, 1.0, 2.0}
	auto list4 = list1 + list1; // 2
	EXPECT_EQ(list0.size(), 4);
	EXPECT_EQ(list0.offset()[4], 12);
	EXPECT_EQ(list1.size(), 1);
	EXPECT_EQ(list2.size(), 5);
	EXPECT_EQ(list3.size(), 5);
	EXPECT_EQ(std::accumulate(list3.data().begin(), list3.data().end(), 0.0), 24 + 18);
	EXPECT_EQ(list4.size(), 2);
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
#define BOX_MSH
#ifdef BOX_MSH
	const std::string filename = "../box.msh";
	int num_entitties[] = {131753, 744285};
	double center[] = {3.7496478658,  -0.005098642278, -0.0188287907804};
	size_t element_num[] = {131753, 0, 3220, 0, 741065, 0, 0, 0, 0};
#else // defined(SPHERE_MSH)
	const std::string filename = "../re3700.msh";
	int num_entitties[] = {1234222, 7326679};
	double center[] = {2.336090445539671,  0.0006215432516590993, 0.00015412152905954366};
	size_t element_num[] = {1234222, 0, 0, 0, 7326679, 0, 0, 0, 0};
#endif

	Mesh<3> mesh;
	MeshIO::read<3>(mesh, filename);

	EXPECT_EQ(mesh.nodes().size(), num_entitties[0] * mesh.dim());
	EXPECT_EQ(mesh.elements().second.size() - mesh.nodes().size() / mesh.dim(), num_entitties[1]);
	const auto nodes = mesh.nodes();
	double x[3] = {0.0, 0.0, 0.0};
	int i = 0;
	
	for(int j = 0; i < nodes.size() / 3; ++j) {
		for(int k = 0; k < mesh.dim(); ++k) {
			x[k] += nodes[j * 3 + k];
		}
		i++;
	}

	EXPECT_NEAR(x[0]/i, center[0], 1e-10);
	EXPECT_NEAR(x[1]/i, center[1], 1e-10);
	EXPECT_NEAR(x[2]/i, center[2], 1e-10);

	// numbers of elements


	const auto& element_list = mesh.elements();
	i = 0;
	const auto& element_type_list = ElementSpace<3>().all_element_types();
	for(const auto type: element_type_list) {
		const auto& elem = mesh.elements(type);
		EXPECT_EQ(elem.first.size(), element_num[i]);
		EXPECT_EQ(elem.second.size(), element_num[i]);
		++i;
	}
}

int main(int argc, char* argv[] ){
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
