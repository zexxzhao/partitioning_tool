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

	std::vector<std::size_t> x5 = {0, 1, 2, 0, 2, 3, 1, 2, 4};
	std::vector<std::size_t> indptr5 = {0, 3, 5, 9};
	CSRList list5(x5, indptr5);

	auto list6 = list5.reverse();
	EXPECT_EQ(list6.data()[0], 0);
	EXPECT_EQ(list6.data()[3], 2);
	EXPECT_EQ(list6.data()[6], 2);
	EXPECT_EQ(std::accumulate(list6.data().begin(), list6.data().end(), 0), 10);
	EXPECT_EQ(std::accumulate(list6.offset().begin(), list6.offset().end(), 0), 30);
}

TEST(CSRList, reverse) {
	std::vector<std::size_t> x0 = {0, 1, 2, 0, 2, 3, 1, 2, 4};
	std::vector<std::size_t> indptr0 = {0, 3, 5, 9};
	// 0->{0, 1, 2}
	// 1->{0, 2}
	// 2->{3, 1, 2, 4}
	// reverse
	// 0 ->{0, 1}
	// 1 ->{0, 2}
	// 2 ->{0, 1, 2}
	// 3 ->{2}
	// 4 ->{2}
	// data = {0, 1, 0, 2, 0, 1, 2, 2, 2}
	// offset = {0, 2, 4, 7, 8, 9}
	auto list = CSRList(x0, indptr0).reverse();
	EXPECT_EQ(std::accumulate(list.data().begin(), list.data().end(), 0), 10);
	EXPECT_EQ(std::accumulate(list.offset().begin(), list.offset().end(), 0), 30);

	std::vector<std::size_t> x1 = {0,2,4,9,6,4,8};
	std::vector<std::size_t> indptr1 = {0,3,3,3,3,3,7};
	auto list1 = CSRList(x1, indptr1);
	// 0 ->{0, 2, 4}
	// 1,2,3,4 ->{}
	// 5 ->{9, 6, 4, 8}
	// reverse
	// 0 -> {0}
	// 1 -> {}
	// 2 -> {0}
	// 3 -> {}
	// 4 -> {0, 5}
	// 5 -> {}
	// 6 -> {5}
	// 7 -> {}
	// 8 -> {5}
	// 9 -> {5}
	// data = {0,0,0,5,5,5,5}
	// offset = {0,1,1,2,2,4,4,5,5,6,7}

	EXPECT_EQ(
		std::accumulate(x1.begin(), x1.end(), 0), 
		std::accumulate(list1.data().begin(), list1.data().end(), 0)
	);
	EXPECT_EQ(
		std::accumulate(indptr1.begin(), indptr1.end(), 0),
		std::accumulate(list1.offset().begin(), list1.offset().end(), 0)
	);
	auto list2 = list1.reverse();
	EXPECT_EQ(std::accumulate(list2.data().begin(), list2.data().end(), 0), 20);
	EXPECT_EQ(std::accumulate(list2.offset().begin(), list2.offset().end(), 0), 37);
	auto list3 = list2.reverse();
	EXPECT_EQ(
		std::accumulate(list1.data().begin(), list1.data().end(), 0),
		std::accumulate(list3.data().begin(), list3.data().end(), 0)
	);
	EXPECT_EQ(
		std::accumulate(list1.offset().begin(), list1.offset().end(), 0),
		std::accumulate(list3.offset().begin(), list3.offset().end(), 0)
	);
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

TEST(MeshIO, Mesh) {


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


TEST(MeshConnectivity, Mesh) {
	Mesh<3> mesh;
	MeshIO::read<3>(mesh, filename);

	MeshConnectivity<3> conn(mesh);
	/*
	for(std::size_t i = 0; i <= 3; ++i) {
		for(std::size_t j = 0; j <= 3; ++j) {
			if (conn._connectivity.find({i, j}) != conn._connectivity.end()) {
				std::cout << "[i, j] = " 
					<< i << ", " << j 
					<< " size = " <<conn._connectivity.at({i, j}).size() << std::endl;
			}
		}
	}
	*/

	EXPECT_EQ( conn.connectivity(3, 0).size(), mesh.elements(FiniteElementType::Tetrahedron).second.size() );
	EXPECT_EQ( conn.connectivity(3, 2).size(), mesh.elements(FiniteElementType::Tetrahedron).second.size());
	EXPECT_EQ( conn.connectivity(2, 0).size(), element_num[2]);
	bool check_0to2 = true;
	bool check_0to3 = true;
	bool check_2to3 = true;
	bool check_3to2 = true;
	bool check_vtx_connectivity = true;
	if(check_0to2){
		const auto& map0to2 = conn.connectivity(0, 2);
		const auto& map2to0 = conn.connectivity(2, 0);
		ASSERT_EQ(map0to2.size(), mesh.elements(FiniteElementType::Vertex).second.size());
		for(std::size_t ivtx = 0; ivtx < map0to2.size(); ++ivtx) {
			for(auto ifacet : map0to2[ivtx]) {
				auto vertex_on_facet = map2to0[ifacet];
				auto found = std::accumulate(
					vertex_on_facet.begin(), 
					vertex_on_facet.end(),
					0,
					[&](int init, std::size_t a) {
						return init + (a == ivtx);
					}
				);
				EXPECT_EQ(found, 1);
			}
		}
	}
	if(check_0to3){
		const auto& map0to3 = conn.connectivity(0, 3);
		const auto& map3to0 = conn.connectivity(3, 0);
		ASSERT_EQ(map0to3.size(), mesh.elements(FiniteElementType::Vertex).second.size());
		for(std::size_t ivtx = 0; ivtx < map0to3.size(); ++ivtx) {
			for(auto ielem : map0to3[ivtx]) {
				auto vertex_on_cell = map3to0[ielem];
				auto found = std::accumulate(
					vertex_on_cell.begin(), 
					vertex_on_cell.end(),
					0,
					[&](int init, std::size_t a) {
						return init + (a == ivtx);
					}
				);
				EXPECT_EQ(found, 1);
			}
		}
	}
	if(check_2to3){
		const auto& map2to3 = conn.connectivity(2, 3);
		const auto& map2to0 = conn.connectivity(2, 0);
		const auto& map3to0 = conn.connectivity(3, 0);
		for(std::size_t ifacet = 0; ifacet < map2to3.size(); ++ifacet) {
			ASSERT_LE(map2to3[ifacet].size(), 2);
			auto ielem = map2to3[ifacet][0];
			auto vertex_on_facet = map2to0[ifacet];
			auto vertex_on_cell = map3to0[ielem];
			int found = std::accumulate(
				vertex_on_facet.begin(), 
				vertex_on_facet.end(), 
				0,
				[&](int init, std::size_t a) {
					return init + (std::find(vertex_on_cell.begin(), vertex_on_cell.end(), a) != vertex_on_cell.end());
				}
			);
			EXPECT_EQ(found, vertex_on_facet.size());
		}
	}
	if(check_3to2) {
		const auto& map3to2 = conn.connectivity(3, 2);
		const auto& map2to0 = conn.connectivity(2, 0);
		const auto& map3to0 = conn.connectivity(3, 0);
		for(std::size_t ielem = 0; ielem < map3to2.size(); ++ielem) {
			if(map3to2[ielem].empty()) {
				continue;
			}
			auto ifacet = map3to2[ielem][0];
			auto vertex_on_facet = map2to0[ifacet];
			auto vertex_on_cell = map3to0[ielem];
			int found = std::accumulate(
				vertex_on_facet.begin(), 
				vertex_on_facet.end(), 
				0,
				[&](int init, std::size_t a) {
					return init + (std::find(vertex_on_cell.begin(), vertex_on_cell.end(), a) != vertex_on_cell.end());
				}
			);
			EXPECT_EQ(found, vertex_on_facet.size());
		}
	}
	if(check_vtx_connectivity) {
		const auto& map0 = conn.adjacent_vertices();
		const auto& map1 = map0.reverse();
		ASSERT_EQ(map0.size(), mesh.nodes().size() / 3);
		ASSERT_EQ(map0.size(), map1.size());

		for(auto i = 0; i < map0.data().size(); ++i) {
			EXPECT_EQ(map0.data()[i], map1.data()[i]);
		}

		for(auto i = 0; i < map0.offset().size(); ++i) {
			EXPECT_EQ(map0.offset()[i], map1.offset()[i]);
		}
	}
}

int main(int argc, char* argv[] ){
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
