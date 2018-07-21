program parser
implicit none
integer (kind = 4) :: physical_names_count;
integer (kind = 4) :: nodes_count;
integer (kind = 4) :: elements_count;

structure /node_info/
    integer (kind = 4) :: number;
    
end structure

print *, "Starting from fortran..."
print *, "Calling C code..."

call parse_input_file(physical_names_count, nodes_count, elements_count)

!
! TODO : allocate memory here based on retrieved array lengths
!

! call retrieve_read_data(physical_names_info, nodes_info, elements_info)

print *, physical_names_count
print *, nodes_count
print *, elements_count
end program parser
