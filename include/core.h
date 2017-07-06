#include <opencv2/opencv.hpp>
#include <vector>

#ifndef CONTENTAWARERESIZE_CORE_H
#define CONTENTAWARERESIZE_CORE_H

namespace core {
    typedef std::vector<cv::Point2i> PVec; // Pixels to add/delete

    class MatWrp {
     private:
        bool transposed;
    public:
        cv::Mat mat;
        MatWrp();
        MatWrp(MatWrp& other);
        MatWrp(MatWrp&& other);
        MatWrp(cv::Mat& other);
        MatWrp(cv::Mat&& other);
        MatWrp(int h, int w, int type);

        MatWrp clone() const;

        const int width() const;
        const int hieght() const;

        template <typename T>
        T& at(int i, int j);

        template <typename T>
        const T& at(int i, int j) const;

        void transpose();
        void set_shape(const MatWrp& other);
        void set_orientation(const MatWrp& other);
        MatWrp  operator() (cv::Range rowRange, cv::Range colRange) const;
        MatWrp& operator= (const MatWrp& other);
        MatWrp& operator= (MatWrp&& other);
    };

    void calc_dynamics(const MatWrp& in, MatWrp& dynamics) {
        for (int i = 0; i < in.hieght(); ++i) {
            dynamics.at<double>(i, 0) = in.at<double>(i, 0);
        }

        for (int curr_col = 1; curr_col < in.width(); ++curr_col) {
            for (int curr_row = 0; curr_row < in.hieght(); ++curr_row) {
                double curr_min = dynamics.at<double>(curr_row, curr_col - 1) + in.at<double>(curr_row, curr_col);
                for (int delta = -1; delta <= 1; ++delta) {
                    if (delta + curr_row < in.hieght() && delta + curr_row >= 0) {
                        if (curr_min > in.at<double>(curr_col, curr_row) +
                                       dynamics.at<double>(curr_row + delta, curr_col - 1)) {
                            curr_min = in.at<double>(curr_row, curr_col) +
                                       dynamics.at<double>(curr_row + delta, curr_col - 1);
                        }
                    }
                }
                dynamics.at<double>(curr_row, curr_col) = curr_min;
            }
        }
    }

    template <typename TFilter>
    PVec low_energy_path(const MatWrp& in, const TFilter& filter) {
        MatWrp grad;
        grad.set_shape(in);
        filter(in.mat, grad.mat);
        MatWrp dynamics(in.hieght(), in.width(), in.mat.type());
        calc_dynamics(grad, dynamics);
        double min = dynamics.at<double>(0, in.width() - 1);
        int min_i = 0;
        for (int i = 0; i < dynamics.hieght(); ++i) {
            if (min > dynamics.at<double>(i, in.width() - 1)) {
                min = dynamics.at<double>(i, in.width() - 1);
                min_i = i;
            }
        }
        int curr_row = min_i;
        int curr_col = in.width() - 1;
        PVec path;
        path.emplace_back(curr_col, curr_row);
        while (curr_col > 0) {
            int suitable_delta = 0;
            double curr_min = 1e8;
            for (int delta = -1; delta <= 1; ++delta) {
                if (delta + curr_row < in.hieght() && delta + curr_row >= 0) {
                    if (curr_min > dynamics.at<double>(curr_row + delta, curr_col)) {
                        curr_min = dynamics.at<double>(curr_row + delta, curr_col);
                        suitable_delta = delta;
                    }
                }
            }
            curr_row += suitable_delta;
            --curr_col;
            path.emplace_back(curr_col, curr_row);
        }
        return path;
    }


    void remove_row(PVec& points, MatWrp& from) {
        for (int i = 0; i != from.width(); ++i) {
            int delta = 0;
            for (int j = 0; j != from.hieght(); ++j) {
                if (points[i].y == j) {
                    delta = -1;
                    continue;
                }
                from.at<cv::Vec3b>(j + delta, i) = from.at<cv::Vec3b>(j, i);
            }
        }
        from = from(cv::Range(0, from.hieght() - 1), cv::Range(0, from.width()));
    }

    // Remove/add k rows to the image
    template <typename TFilter> // Class with overrided operator()
    void remove_rows(MatWrp& in, int k, const TFilter& filter) {
        for (int l = 0; l != k; ++l) {
            auto pixels_to_remove = low_energy_path(in, filter);
            remove_row(pixels_to_remove, in);
        }
    }

    // Change image size to desirable
    // shrink_to_fit of image (640 x 480) with new_size (600 x 500) returns image (600 x 480)
    // expand_to_fit of image (640 x 480) with new_size (600 x 500) returns image (640 x 500)
    template <typename TFilter>
    void shrink_to_fit(const cv::Mat& in, cv::Mat& out, const cv::Size& new_size, const TFilter& filter) {
        cv::Size in_size = in.size();
        MatWrp in_wrp(in.clone());
        MatWrp out_wrp;
        if (in_size.height > new_size.height) {
            remove_rows(in_wrp, in_size.height - new_size.height, filter);
        }
        if (in_size.width > new_size.width) {
            in_wrp.transpose();
            remove_rows(in_wrp, in_size.width - new_size.width, filter);
        }
        out = in_wrp.mat;
    }

    template <typename TFilter>
    void expand_to_fit(const cv::Mat& in, cv::Mat& out, const cv::Size& new_size, const TFilter& filter) {
        // TODO
    }

}

#endif //CONTENTAWARERESIZE_CORE_H
