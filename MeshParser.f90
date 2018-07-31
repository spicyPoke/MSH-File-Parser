program parser
use, intrinsic :: ISO_C_BINDING
implicit none

integer (kind = 4) :: physical_names_count;
integer (kind = 4) :: nodes_count;
integer (kind = 4) :: elements_count;

integer (kind = 4) :: i

type node_info
    integer (kind = 4) :: number;
    real    (kind = 4) :: x;
    real    (kind = 4) :: y;
    real    (kind = 4) :: z;    
end type node_info
type(node_info), allocatable, dimension(:) :: nodes_info

type element_info
    integer (kind = 4) :: number;
    integer (kind = 4) :: type;
    integer (kind = 4) :: tag_count;
    integer (kind = 4), dimension(10) :: tags
    integer (kind = 4), dimension(8) :: nodes
end type element_info
type(element_info), allocatable, dimension(:) :: elements_info

type physical_name_info
    integer (kind = 4) :: dimension
    integer (kind = 4) :: physical_number
    character (len = 1), dimension(32) :: name
end type physical_name_info
type(physical_name_info), allocatable, dimension(:) :: physical_names_info

type ine_info

    integer (kind = 4) :: indicator;
    integer (kind = 4), dimension(2) :: nodes;
    integer (kind = 4) :: element;
    integer (kind = 4) :: edge_number;
end type ine_info
type(ine_info), allocatable, dimension(:) :: ines_info

print *, "Starting from fortran..."
print *, "Calling C code..."

call parse_input_file(physical_names_count, nodes_count, elements_count)
call label_the_edges()

allocate(elements_info(elements_count))
allocate(nodes_info(nodes_count))
allocate(ines_info(elements_count * 3))

do i = 1, elements_count
    call retrieve_element_info(i, elements_info(i)%number, elements_info(i)%type, &
     elements_info(i)%tag_count, elements_info(i)%tags, elements_info(i)%nodes)
    print *, i, "element ", elements_info(i)%number , "[", elements_info(i)%nodes(1), &
     ", ", elements_info(i)%nodes(2), ", ", elements_info(i)%nodes(3), "]"  
end do

do i = 1, nodes_count
    call retrieve_node_info(i, nodes_info(i)%number, nodes_info(i)%x, nodes_info(i)%y, nodes_info(i)%z)
    print *, i, "node ", nodes_info(i)%number , "[", nodes_info(i)%x, ", ", nodes_info(i)%y, ", ", nodes_info(i)%z, "]"  
end do

do i = 1, (elements_count * 3)
    call retrieve_ine_array(i, ines_info(i)%indicator, ines_info(i)%nodes, &
    ines_info(i)%element, ines_info(i)%edge_number)
    print *, i, "indicator ", ines_info(i)%indicator , "[", ines_info(i)%nodes(1), &
    ", ", ines_info(i)%nodes(2), ", ", ines_info(i)%element, ", ", ines_info(i)%edge_number,"]"  
end do

print *, physical_names_count
print *, nodes_count
print *, elements_count
end program parser
