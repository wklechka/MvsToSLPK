#include "pch.h"
#include "ProjectDivision.h"
#include "StdUtil/StdUtility.h"
#include "StdUtil/WinUtility.h"
#include "StdUtil/MathFunc.h"
#include "SmtPrj/SmtPrj.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <functional>

#include <cstdlib>
#include <ctime>

#include "Eigen/Geometry"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif


// Image structure with XY coordinates
struct QImage {
	double x, y;             // Image center coordinates
	std::string name;
};

class Quadtree {
protected:
	int maxImages;          // Maximum images per node (only for leaf nodes)
	int minImages;          // Minimum required images per region (for redistribution checks)
	double overlapRatio;    // Percentage overlap (e.g., 0.2 for 20%)
	double xMin, xMax, yMin, yMax; // Bounding box of this node
	std::vector<QImage> images;   // Stored images (only when this is a leaf)
	Quadtree* NW = nullptr;
	Quadtree* NE = nullptr;
	Quadtree* SW = nullptr;
	Quadtree* SE = nullptr;
	Quadtree* parent = nullptr;  // Pointer needed for redistribution

public:
	// Constructor accepts maxImages, minImages, overlapRatio, and an optional parent pointer.
	Quadtree(double xMin, double xMax, double yMin, double yMax,
		int maxImages, int minImages, double overlapRatio, Quadtree* parent = nullptr)
		: xMin(xMin), xMax(xMax), yMin(yMin), yMax(yMax),
		maxImages(maxImages), minImages(minImages), overlapRatio(overlapRatio), parent(parent) {
	}

	~Quadtree() {
		delete NW;
		delete NE;
		delete SW;
		delete SE;
	}

	double getXMin() const { return xMin; }
	double getXMax() const { return xMax; }
	double getYMin() const { return yMin; }
	double getYMax() const { return yMax; }
	size_t getImageCount() const { return images.size(); }
	const std::vector<QImage>& getImageList() const { return images; }

	// Returns true if this node is a leaf (i.e., has no children.)
	bool isLeaf() const {
		return NW == nullptr;
	}

	// Check whether the image falls within this node’s region (with additional overlap)
	bool overlaps(const QImage& img) const {
		double xBuffer = (xMax - xMin) * overlapRatio;
		double yBuffer = (yMax - yMin) * overlapRatio;
		return img.x >= (xMin - xBuffer) && img.x <= (xMax + xBuffer) &&
			img.y >= (yMin - yBuffer) && img.y <= (yMax + yBuffer);
	}

	// Insert an image into this quadtree.
	// Once subdivided, new images are always pushed to the children.
	void insert(const QImage& img) {
		if (!overlaps(img))
			return; // Skip if the image does not fall within this (expanded) region.

		// If the node is subdivided, pass the image to the children.
		if (!isLeaf()) {
			if (NW->overlaps(img)) NW->insert(img);
			if (NE->overlaps(img)) NE->insert(img);
			if (SW->overlaps(img)) SW->insert(img);
			if (SE->overlaps(img)) SE->insert(img);
			return;
		}

		// We are in a leaf node; if there's room, add the image.
		if (images.size() < static_cast<size_t>(maxImages)) {
			images.push_back(img);
			return;
		}

		// Capacity reached: subdivide and then push the new image into the children.
		subdivide();
		if (NW->overlaps(img)) NW->insert(img);
		if (NE->overlaps(img)) NE->insert(img);
		if (SW->overlaps(img)) SW->insert(img);
		if (SE->overlaps(img)) SE->insert(img);
	}

	// Subdivide the current node into four children and redistribute any stored images.
	void subdivide() {
		double midX = (xMin + xMax) / 2.0;
		double midY = (yMin + yMax) / 2.0;

		NW = new Quadtree(xMin, midX, midY, yMax, maxImages, minImages, overlapRatio, this);
		NE = new Quadtree(midX, xMax, midY, yMax, maxImages, minImages, overlapRatio, this);
		SW = new Quadtree(xMin, midX, yMin, midY, maxImages, minImages, overlapRatio, this);
		SE = new Quadtree(midX, xMax, yMin, midY, maxImages, minImages, overlapRatio, this);

		// Redistribute the images stored in this node into the proper child nodes.
		for (const auto& img : images) {
			if (NW->overlaps(img)) NW->insert(img);
			if (NE->overlaps(img)) NE->insert(img);
			if (SW->overlaps(img)) SW->insert(img);
			if (SE->overlaps(img)) SE->insert(img);
		}
		images.clear(); // Clear images at this node as it's no longer used.
	}

	// Enforce that each leaf node holds at least the minimum required images.
	// If a leaf node doesn't, redistribute its images to its siblings.
	void enforceMinimumImages() {
		if (!isLeaf()) {
			NW->enforceMinimumImages();
			NE->enforceMinimumImages();
			SW->enforceMinimumImages();
			SE->enforceMinimumImages();
		}
		else {
			if (images.size() < static_cast<size_t>(minImages))
				redistributeImagesToSiblings();
		}
	}

	// Check if a given vector of images already contains an image with the same name.
	bool containsImage(const std::vector<QImage>& imgs, const QImage& img) const {
		for (const auto& existingImg : imgs) {
			if (existingImg.name == img.name)
				return true;
		}
		return false;
	}

	void insertAt(const Quadtree* sibling, const QImage& img) {

		// If the node is subdivided, pass the image to the children.
		if (!isLeaf()) {
			if (NW->overlaps(img)) NW->insert(img);
			if (NE->overlaps(img)) NE->insert(img);
			if (SW->overlaps(img)) SW->insert(img);
			if (SE->overlaps(img)) SE->insert(img);
			return;
		}

		images.push_back(img);
	}

	 // Redistribute images of this underfilled leaf node to the closest sibling node
	// (from parent's children) that qualifies.
	void redistributeImagesToSiblings() {
		if (!parent)
			return; // Cannot redistribute if there is no parent.

		std::vector<QImage> tempImages = images;
		images.clear();

		// Compute the center of the current node.
		double currentCenterX = (xMin + xMax) / 2.0;
		double currentCenterY = (yMin + yMax) / 2.0;

		// We'll check among parent's children.
		Quadtree* siblings[4] = { parent->NW, parent->NE, parent->SW, parent->SE };

		for (const auto& img : tempImages) {
			Quadtree* bestCandidate = nullptr;
			double bestDistSq = std::numeric_limits<double>::infinity();

			for (int i = 0; i < 4; i++) {
				Quadtree* candidate = siblings[i];
				if (!candidate || candidate == this)
					continue;
				// Only consider candidate if it is a leaf, has at least minImages, and does not already contain the image.
 				if (!candidate->isLeaf() && candidate->images.size() == 0)
 					continue;
				if (candidate->containsImage(candidate->images, img))
					continue;

				// Compute candidate's center.
				double candidateCenterX = (candidate->xMin + candidate->xMax) / 2.0;
				double candidateCenterY = (candidate->yMin + candidate->yMax) / 2.0;
				double dx = candidateCenterX - currentCenterX;
				double dy = candidateCenterY - currentCenterY;
				double distSq = dx * dx + dy * dy;

				if (distSq < bestDistSq) {
					bestDistSq = distSq;
					bestCandidate = candidate;
				}
			}

			if (bestCandidate) {
				bestCandidate->insertAt(bestCandidate, img);
			}
			else {
				// If no candidate qualifies, restore the image to the current node.
				//images.push_back(img);
			}
		}
	}

	// Print the quadtree contents to the given output stream with indentation level.
	void print(std::ostream& os, int level) const {
		if (!images.empty()) {
			os << std::fixed << std::setprecision(3);
			os << "Region (" << xMin << ", " << yMin << ") to ("
				<< xMax << ", " << yMax << ") - Images: " << images.size()
				<< " Level=" << level << "\n";
			for (const auto& img : images) {
				os << "  - " << img.name << " at (" << img.x << ", " << img.y << ")\n";
			}
		}

		++level;
		if (NW) NW->print(os, level);
		if (NE) NE->print(os, level);
		if (SW) SW->print(os, level);
		if (SE) SE->print(os, level);
	}

	// NEW: Traverse the quadtree, calling the visitor function on each leaf node.
	void traverseLeaves(const std::function<void(const Quadtree*)>& visitor) const {
		if (isLeaf()) {
			visitor(this);
		}
		else {
			if (NW) NW->traverseLeaves(visitor);
			if (NE) NE->traverseLeaves(visitor);
			if (SW) SW->traverseLeaves(visitor);
			if (SE) SE->traverseLeaves(visitor);
		}
	}

	size_t nodeCount() const {
		size_t count = 1;  // Count the current node.
		if (!isLeaf()) {
			count += NW->nodeCount();
			count += NE->nodeCount();
			count += SW->nodeCount();
			count += SE->nodeCount();
		}
		return count;
	}
	size_t leafCount() const {
		if (isLeaf())
			return 1;
		size_t count = 0;
		if (NW) count += NW->leafCount();
		if (NE) count += NE->leafCount();
		if (SW) count += SW->leafCount();
		if (SE) count += SE->leafCount();
		return count;
	}

};

void quadTreeTest() 
{
	int maxImages = 15;      // Max images per node before splitting
	int minImages = 6;      // Minimum images required per region
	double overlapPercentage = 0.2;

	Quadtree tree(0, 100, 0, 100, maxImages, minImages, overlapPercentage); // Root region

	std::vector<QImage> images;

	// Seed random generator
	//std::srand(std::time(nullptr));
	std::srand(0);

	// Generate 50 random images within (0,100) range
	for (int i = 0; i < 50; ++i) {
		double x = std::rand() % 101; // Random X (0-100)
		double y = std::rand() % 101; // Random Y (0-100)
		images.push_back({ x, y, "Image_" + std::to_string(i + 1) });
	}

	// Insert images into the quadtree
	for (const auto& img : images) {
		tree.insert(img);
	}

	std::ofstream fout;
	fout.open("j:\\tmp\\quadtree.txt");

	tree.print(fout, 0); // Output final quadtree contents

	tree.enforceMinimumImages(); // Apply post-processing step

	fout << std::endl << std::endl;
	fout << "After enforce min" << std::endl;
	tree.print(fout, 0);

	// Traverse leaf nodes and print their region and image count.
	fout << "\nTraversing leaf nodes:\n";
	tree.traverseLeaves([&](const Quadtree* leaf) {
		if (leaf->getImageCount() > 0) {
			fout << "Leaf region (" << leaf->getXMin() << ", " << leaf->getYMin() << ") to ("
				<< leaf->getXMax() << ", " << leaf->getYMax() << ") with "
				<< leaf->getImageCount() << " images.\n";
		}
	});

	fout << "Total nodes in the quadtree: " << tree.nodeCount() << std::endl;
	fout << "Total leaf count: " << tree.leafCount() << std::endl;
}


class ImageOrientation
{
public:
	void setUp(const ISmtPrj::ImageInfo& eoInfo, const ISmtPrj::CameraParameters& camInfo)
	{
		this->eoInfo = eoInfo;
		this->camInfo = camInfo;

		if (this->eoInfo.angleUnits == ISmtPrj::DEGREES) {
			this->eoInfo.omega = MathFunc2::degToRad(this->eoInfo.omega);
			this->eoInfo.phi = MathFunc2::degToRad(this->eoInfo.phi);
			this->eoInfo.kappa = MathFunc2::degToRad(this->eoInfo.kappa);
		}
		else if (this->eoInfo.angleUnits == ISmtPrj::GRADS) {
			this->eoInfo.omega = MathFunc2::gradToRad(this->eoInfo.omega * 0.9);
			this->eoInfo.phi = MathFunc2::gradToRad(this->eoInfo.phi * 0.9);
			this->eoInfo.kappa = MathFunc2::gradToRad(this->eoInfo.kappa * 0.9);
		}
		this->eoInfo.angleUnits = ISmtPrj::RADIANS;

		calculateOrientation();
	}

	bool pixelToPhoto(const Eigen::Vector2d& pixelCoords,
		Eigen::Vector2d& photoCoords) const
	{
		photoCoords[0] = mA0 + mA1 * pixelCoords[0] - mA2 * pixelCoords[1];
		photoCoords[1] = mB0 + mB1 * pixelCoords[0] - mB2 * pixelCoords[1];
		return true;
	}

	bool photoToGround(const Eigen::Vector2d& photoCoords,
		Eigen::Vector3d& groundCoords,
		const double groundZ) const
	{
		double focalLength = camInfo.focal;

		double tmp = mOrg(0, 2) * photoCoords[0] + mOrg(1, 2) * photoCoords[1] - mOrg(2, 2) * focalLength;

		if (tmp == 0.)
		{
			return false;
		}

		double namta = (groundZ - eoInfo.projectionCenterZ) / tmp;

		double gndX = eoInfo.projectionCenterX + namta * (mOrg(0, 0) * photoCoords[0] + mOrg(1, 0) * photoCoords[1] - mOrg(2, 0) * focalLength);
		double gndY = eoInfo.projectionCenterY + namta * (mOrg(0, 1) * photoCoords[0] + mOrg(1, 1) * photoCoords[1] - mOrg(2, 1) * focalLength);

		groundCoords[0] = gndX;
		groundCoords[1] = gndY;
		groundCoords[2] = groundZ;

		return true;
	}

	bool pixelToGround(const Eigen::Vector2d& pixelCoords,
		Eigen::Vector3d& groundCoords,
		const double groundZ) const
	{
		Eigen::Vector2d photoCoords;
		pixelToPhoto(pixelCoords, photoCoords);

		return photoToGround(photoCoords, groundCoords, groundZ);
	}

protected:
	ISmtPrj::ImageInfo eoInfo;
	ISmtPrj::CameraParameters camInfo;

	double	mA0, mA1, mA2, mB0, mB1, mB2;
	Eigen::Matrix3d mOrg;

	void calculateOrientation()
	{
		// Calculate the orientation based on the EO info and camera parameters.
		// You would typically convert omega, phi, kappa to a rotation matrix or quaternion.

		mOrg = Eigen::AngleAxisd(eoInfo.omega, Eigen::Vector3d::UnitX())
			* Eigen::AngleAxisd(eoInfo.phi, Eigen::Vector3d::UnitY())
			* Eigen::AngleAxisd(eoInfo.kappa, Eigen::Vector3d::UnitZ());

		// calculate interior
		double pixelCenterX, pixelCenterY;
		double photoCenterX, photoCenterY;
		double Trans1, Trans2, Trans3, Trans4, Trans5, Trans6, Trans7;

		pixelCenterX = pixelCenterY = 0.0;
		photoCenterX = photoCenterY = 0.0;
		Trans1 = Trans2 = Trans3 = Trans4 = Trans5 = Trans6 = Trans7 = 0.0;

		double pWidth = camInfo.filmWidth / 2.0;
		double pHeight = camInfo.filmHeight / 2.0;

		std::vector <Eigen::Vector2d> pixelCorners(4);
		std::vector <Eigen::Vector2d> photoCorners(4);

		double iW = static_cast<double>(eoInfo.imageWidth - 1);
		double iH = static_cast<double>(eoInfo.imageHeight - 1);

		pixelCorners[0] = Eigen::Vector2d(0.0, iH);
		pixelCorners[1] = Eigen::Vector2d(iW, 0.0);
		pixelCorners[2] = Eigen::Vector2d(0.0, 0.0);
		pixelCorners[3] = Eigen::Vector2d(iW, iH);

		photoCorners[0] = Eigen::Vector2d(-pWidth, -pHeight);
		photoCorners[1] = Eigen::Vector2d(pWidth, pHeight);
		photoCorners[2] = Eigen::Vector2d(-pWidth, pHeight);
		photoCorners[3] = Eigen::Vector2d(pWidth, -pHeight);

		for (int i = 0; i < pixelCorners.size(); i++)
		{
			pixelCenterX += pixelCorners[i][0];
			pixelCenterY += pixelCorners[i][1];
			photoCenterX += photoCorners[i][0];
			photoCenterY += photoCorners[i][1];
		}
		pixelCenterX /= pixelCorners.size();
		pixelCenterY /= pixelCorners.size();
		photoCenterX /= pixelCorners.size();
		photoCenterY /= pixelCorners.size();

		for (int i = 0; i < pixelCorners.size(); i++)
		{
			double tmp1x = pixelCorners[i][0] - pixelCenterX;
			double tmp1y = pixelCorners[i][1] - pixelCenterY;
			double tmp2x = photoCorners[i][0] - camInfo.ppX - photoCenterX;
			double tmp2y = photoCorners[i][1] - camInfo.ppY - photoCenterY;

			Trans1 += tmp1x * tmp1x;
			Trans2 += tmp1y * tmp1y;
			Trans3 += tmp1x * tmp1y;
			Trans4 += tmp1x * tmp2x;
			Trans5 += tmp1y * tmp2x;
			Trans6 += tmp1x * tmp2y;
			Trans7 += tmp1y * tmp2y;
		}

		mA1 = (Trans2 * Trans4 - Trans3 * Trans5) / (Trans1 * Trans2 - Trans3 * Trans3);
		mA2 = -(Trans1 * Trans5 - Trans3 * Trans4) / (Trans1 * Trans2 - Trans3 * Trans3);
		mB1 = (Trans2 * Trans6 - Trans3 * Trans7) / (Trans1 * Trans2 - Trans3 * Trans3);
		mB2 = -(Trans1 * Trans7 - Trans3 * Trans6) / (Trans1 * Trans2 - Trans3 * Trans3);
		mA0 = photoCenterX - mA1 * pixelCenterX + mA2 * pixelCenterY;
		mB0 = photoCenterY - mB1 * pixelCenterX + mB2 * pixelCenterY;
	}
};

bool ProjectDivision::divideProject(ProjectDiv& opt)
{
	auto smtPrj = std::shared_ptr<ISmtPrj>(createISmtPrj(), [](ISmtPrj* p) { p->Release(); });

	if (!smtPrj) {
		return false;
	}

	std::string smtxmlFilename = StdUtility::convert(opt.filename);
	std::string workingDir = StdUtility::convert(opt.workingFolder);
	StdUtility::appendSlash(workingDir);
	workingDir += "Projects\\";

	if (!smtPrj->setFile(smtxmlFilename.c_str())) {
		return false;
	}

	if (smtPrj->numImages() < opt.maxImages) {
		// no divisions needed
		return false;
	}

	// make boxes and divide the project

#if 0
	// first get the project extents
	double xMin = std::numeric_limits<double>::max();
	double xMax = std::numeric_limits<double>::lowest();
	double yMin = std::numeric_limits<double>::max();
	double yMax = std::numeric_limits<double>::lowest();
	for (auto& imageInfo : smtPrj->getImageInfos()) {
		xMin = std::min(xMin, imageInfo.projectionCenterX);
		xMax = std::max(xMax, imageInfo.projectionCenterX);
		yMin = std::min(yMin, imageInfo.projectionCenterY);
		yMax = std::max(yMax, imageInfo.projectionCenterY);
	}

	int maxImages = opt.maxImages;      // Max images per node before splitting
	int minImages = opt.minImages;      // Minimum images required per region
	double overlapPercentage = opt.overlapPercentage;

	Quadtree tree(xMin, xMax, yMin, yMax, maxImages, minImages, overlapPercentage); // Root region
	
	for (auto& imageInfo : smtPrj->getImageInfos()) {
		QImage newImage;
		newImage.name = imageInfo.imageFile;
		newImage.x = imageInfo.projectionCenterX;
		newImage.y = imageInfo.projectionCenterY;
		tree.insert(newImage);
	}
#else

	double aveElevation = smtPrj->averageGroundZ();

		// first get the project extents
	double xMin = std::numeric_limits<double>::max();
	double xMax = std::numeric_limits<double>::lowest();
	double yMin = std::numeric_limits<double>::max();
	double yMax = std::numeric_limits<double>::lowest();
	for (auto& imageInfo : smtPrj->getImageInfos()) {

		ImageOrientation imgOrient;
		ISmtPrj::CameraParameters cam;
		smtPrj->getCameraInfo(imageInfo.assignedCamera, cam);
		imgOrient.setUp(imageInfo, cam);

		Eigen::Vector3d groundCoords;
		Eigen::Vector2d photoCoords(0.0, 0.0);
		if (!imgOrient.photoToGround(photoCoords, groundCoords, aveElevation)) {
			continue; // skip if conversion fails
		}

		xMin = std::min(xMin, groundCoords[0]);
		xMax = std::max(xMax, groundCoords[0]);
		yMin = std::min(yMin, groundCoords[1]);
		yMax = std::max(yMax, groundCoords[1]);
	}

	int maxImages = opt.maxImages;      // Max images per node before splitting
	int minImages = opt.minImages;      // Minimum images required per region
	double overlapPercentage = opt.overlapPercentage;

	Quadtree tree(xMin, xMax, yMin, yMax, maxImages, minImages, overlapPercentage); // Root region

	for (auto& imageInfo : smtPrj->getImageInfos()) {

		ImageOrientation imgOrient;
		ISmtPrj::CameraParameters cam;
		smtPrj->getCameraInfo(imageInfo.assignedCamera, cam);
		imgOrient.setUp(imageInfo, cam);

		Eigen::Vector3d groundCoords;
		Eigen::Vector2d photoCoords(0.0, 0.0);
		if (!imgOrient.photoToGround(photoCoords, groundCoords, aveElevation)) {
			continue; // skip if conversion fails
		}

		QImage newImage;
		newImage.name = imageInfo.imageFile;
		newImage.x = groundCoords[0];
		newImage.y = groundCoords[1];
		tree.insert(newImage);
	}
#endif


	tree.enforceMinimumImages();

	if (tree.leafCount() == 1) {
		return false;
	}

	StdUtility::createFullDirectoryPath(workingDir);

	int divNum = 0;
	auto traverseFunc = [&](const Quadtree* leaf) 
	{
		if (leaf->isLeaf()) {
			if (leaf->getImageCount() > 0) {
				// make projects
				auto newSmtPrj = std::shared_ptr<ISmtPrj>(createISmtPrj(), [](ISmtPrj* p) { p->Release(); });

				if (!newSmtPrj->setFile(smtxmlFilename.c_str())) {
					return;
				}

				std::vector<ISmtPrj::ImageInfo> newList;

				for (auto& img : leaf->getImageList()) {
					for (auto& imageInfo : newSmtPrj->getImageInfos()) {
						if (imageInfo.imageFile == img.name) {
							newList.push_back(imageInfo);
							break; // found the image, no need to continue
						}
					}
				}

				newSmtPrj->getImageInfos() = newList;
				newSmtPrj->forceValidModelList();

				std::string newFileName = workingDir;
				newFileName += StdUtility::getName(smtxmlFilename);
				newFileName += "_" + std::to_string(divNum) + ".smtxml";

				newSmtPrj->writeProject(newFileName.c_str());
				opt.projectsGenerated.push_back(newFileName);
				++divNum;
			}
		}
	};

	tree.traverseLeaves(traverseFunc);


	if (opt.showQuadtree) {
		std::ofstream fout;

		std::string outFilename = workingDir;
		StdUtility::appendSlash(outFilename);
		outFilename += "quadtree.txt";

		fout.open(outFilename.c_str());

		tree.print(fout, 0); // Output final quadtree contents

		tree.enforceMinimumImages(); // Apply post-processing step

		fout << std::endl << std::endl;
		fout << "After enforce min:" << std::endl;
		tree.print(fout, 0);

		// Traverse leaf nodes and print their region and image count.
		fout << "\nTraversing leaf nodes:\n";
		tree.traverseLeaves([&](const Quadtree* leaf) {
			if (leaf->getImageCount() > 0) {
				fout << std::fixed << std::setprecision(3);
				fout << "Leaf region (" << leaf->getXMin() << ", " << leaf->getYMin() << ") to ("
					<< leaf->getXMax() << ", " << leaf->getYMax() << ") with "
					<< leaf->getImageCount() << " images.\n";
			}
			});

		fout << "Total nodes in the quadtree: " << tree.nodeCount() << std::endl;
		fout << "Total leaf count: " << tree.leafCount() << std::endl;
	}

	return opt.projectsGenerated.size() > 1;
}

