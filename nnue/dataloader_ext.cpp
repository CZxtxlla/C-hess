#include <torch/extension.h>
#include <vector>
#include <string>
#include "dataloader.h" 

class PyChessDataLoader {
private:
    FILE* file;
    SparseBatch* batch;
    int batch_size;

public:
    PyChessDataLoader(const std::string& filepath, int batch_size) {
        file = fopen(filepath.c_str(), "rb");
        if (!file) throw std::runtime_error("Could not open dataset file.");
        
        this->batch_size = batch_size;
        batch = create_sparse_batch(batch_size);
    }

    ~PyChessDataLoader() {
        if (file) fclose(file);
        if (batch) {
            // Free the new perspective arrays
            free(batch->w_row_indices);
            free(batch->w_col_indices);
            free(batch->b_row_indices);
            free(batch->b_col_indices);
            free(batch->targets);
            free(batch);
        }
    }

    // Returns: [w_row, w_col, b_row, b_col, targets]
    std::vector<torch::Tensor> next_batch() {
        if (!load_next_batch(file, batch)) {
            return {}; // Return empty list on End of File
        }

        auto options_int = torch::TensorOptions().dtype(torch::kInt32);
        auto options_float = torch::TensorOptions().dtype(torch::kFloat32);

        // White perspective tensors (using w_nnz)
        torch::Tensor w_row = torch::from_blob(batch->w_row_indices, {batch->current_w_nnz}, options_int).clone();
        torch::Tensor w_col = torch::from_blob(batch->w_col_indices, {batch->current_w_nnz}, options_int).clone();
        
        // Black perspective tensors (using b_nnz)
        torch::Tensor b_row = torch::from_blob(batch->b_row_indices, {batch->current_b_nnz}, options_int).clone();
        torch::Tensor b_col = torch::from_blob(batch->b_col_indices, {batch->current_b_nnz}, options_int).clone();

        // Targets
        torch::Tensor targets = torch::from_blob(batch->targets, {batch->batch_size}, options_float).clone();

        return {w_row, w_col, b_row, b_col, targets};
    }
};

// Bind the C++ class to Python
PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
    py::class_<PyChessDataLoader>(m, "FastDataLoader")
        .def(py::init<std::string, int>())
        .def("next_batch", &PyChessDataLoader::next_batch);
}