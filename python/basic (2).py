# Define a grid for numbers
number_grid = [
[1, 3, 1, 5, 1, 6, 43, 16, 33, 1],
[12, 30, 11, 23, 17, 7, 25, 18, 2, 4],
[3, 5, 38, 34, 21, 9, 23, 14, 32, 35],
[37, 38, 24, 31, 33, 41, 25, 17, 8, 7],
[23, 25, 22, 14, 34, 42, 26, 21, 44, 18],
[13, 4, 2, 9, 33, 15, 19, 17, 42, 9],
[17, 22, 41, 5, 4, 26, 21, 22, 38, 39],
[18, 18, 38, 36, 6, 30, 35, 33, 17, 19],
[15, 29, 6, 28, 26, 24, 20, 40, 10, 27],
[34, 45, 45, 34, 34, 44, 45, 34, 45, 5]
]
annotation_grid=[
[False, False, False, False, True, False, False, False, True, False],
[False, False, True, False, False, False, False, False, False, False],
[True, False, False, False, False, False, False, False, False, False],
[False, False, False, False, False, False, False, False, False, False],
[False, False, False, False, True, False, False, False, True, False],
[False, True, False, False, False, False, False, False, False, False],
[False, False, False, False, False, True, False, False, False, True],
[False, False, False, False, False, False, False, False, False, False],
[False, True, False, False, False, False, True, False, False, False],
[False, False, False, False, False, False, False, False, False, False]
]


def is_coordinate_adjacent(coord_list, new_coord):
    for coord in coord_list[:-1]:
        x_diff = abs(coord[0] - new_coord[0])
        y_diff = abs(coord[1] - new_coord[1])
        if (x_diff == 1 and y_diff == 0) or (x_diff == 0 and y_diff == 1):
            return True
    return False


def find_snake_path(number_grid, annotation_grid, path, values, position, value):
    # position is a tuple: (x, y)
    x, y = position
      # Check if position is out of the grid or has already been visited
    if x < 0 or y < 0 or x >= len(number_grid) or y >= len(number_grid[0]) or (x, y) in path:
        return False
    
    newvalue =number_grid[x][y] 

   
    if(False and len(values)):
        lastvalue = values[-1]
        if abs(newvalue-lastvalue)<5:
            return False


    if is_coordinate_adjacent(path, position):
        return False
    
    if newvalue in values:
        return False

    # Add this position to the path
    path.append((x, y))
    values.append(newvalue)

   


    # If we've reached the end of the path, stop
    if value == 45 and check_annotated_cells(path, annotation_grid):
        return True

    # Check all possible directions
    for dx, dy in [(0, 1), (0, -1), (1, 0), (-1, 0)]:
        if find_snake_path(number_grid, annotation_grid, path,values, (x + dx, y + dy), value + 1):
            return True

    # If we've reached here, none of the directions worked. Backtrack.
    path.pop()
    values.pop()
    return False

def check_annotated_cells(path, annotation_grid):
    # Check if all annotated cells are included in the path
    for i in range(len(annotation_grid)):
        for j in range(len(annotation_grid[0])):
            if annotation_grid[i][j] and (i, j) not in path:
                return False
    return True

# Define your number_grid and annotation_grid here
path = []
values = []
start_position = (0, 4)  # Replace with the actual start position
if find_snake_path(number_grid, annotation_grid, path, values, start_position, 1):
    print("Solution found:", path)
else:
    print("No solution found")