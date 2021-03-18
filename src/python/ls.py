from tree import PNode, PTree
from queries import query
from mst import prims_mst
from dist import mat_to_func
def local_search_with(query_func):
    """
        Using QUERY_FUNC to determine which direction to go locally,
        return local_search function that uses it
        
        interface:
            QUERY_FUNC(insert_leaf, internal_node, D)
    """
    def local_search(insert_leaf, D, start_leaf):
        """
            Starting from START_LEAF in the tree, do local hill climbing search
                to find the best node,
            use distance matrix D in sub calls to node queries
            
            return the edge to break in order to insert INSERT_LEAF
        """
        visited = set()
        # don't actually start at a leaf but at an adjacent internal node
        curr_node = start_leaf.get_neighbors()[0]
        curr_neighbors = curr_node.get_neighbors()
        neighbors_not_visited = set(curr_neighbors) - visited
        while len(neighbors_not_visited) > 1:
            #if curr_node.is_leaf():
            #    raise RuntimeError("Bad")
            best_neighbor = query_func(insert_leaf,curr_node, D)
            visited.update(set(curr_neighbors) - set([best_neighbor]))
            prev_node = curr_node
            curr_node = best_neighbor
            if curr_node.is_leaf():
                break
            curr_neighbors = curr_node.get_neighbors()
            neighbors_not_visited = set(curr_neighbors) - visited
        # return the edge between curr_node and prev_node
        return (curr_node, prev_node)
    return local_search

def LS(names, matrix, start_close=True):
    """
        estimate a tree using 'local search', where in each insertion we start at a leaf
        and use node queries until we're told to backtrack.
        
        noisy_dist_matrix - numpy matrix describing interleaf distances
        start_close - for each leaf to insert, start at the already-inserted leaf D-closest to the insert
        return - a newick string of the tree
    """
    D = mat_to_func(names, matrix)
    if start_close:
        _,_,insert_order, start_leaf_order = prims_mst(names,D, True)
    else:
        _, _, insert_order = prims_mst(names,D, False)

    names2nodes = dict()
    for name in insert_order:
        names2nodes[name] = PNode(name)
    ptree = PTree(search_fn=local_search_with(query))
    for i in range(len(insert_order)):
        next_leaf = names2nodes[insert_order[i]]
        start_leaf = ptree.get_root()
        if start_close:
            if not start_leaf_order[i]:
                # For the first "None", start_leaf = None
                start_leaf = None
            else:
                start_leaf = names2nodes[start_leaf_order[i]]
        ptree.insert_leaf(next_leaf, D, start_leaf=start_leaf)
    return ptree.make_newick_string()
