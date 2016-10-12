#include <random>
#include <vector>
#include <algorithm>
#include <functional>

namespace pf
{
	template<typename FLT_TYPE = float> class particleBase
	{
	public:
		virtual FLT_TYPE &operator[](const size_t i) = 0;
		virtual const size_t size() = 0;
		template <typename T> T operator+(const T &a)
		{
			T in = a;
			T ret;
			for(size_t i = 0; i < size(); i ++)
			{
				ret[i] = (*this)[i] + in[i];
			}
			return ret;
		}
	private:
	};

	template<typename T, typename FLT_TYPE = float> class particle_filter
	{
	public:
		particle_filter(const int nParticles):
			engine(seed_gen())
		{
			particles.resize(nParticles);
			ind_histogram.resize(ie.size());
			for(size_t i = 0; i < ie.size(); i ++)
			{
				ind_histogram[i].resize(nParticles);
			}
		};
		T generate_noise(T mean, T sigma)
		{
			T noise;
			for(size_t i = 0; i < ie.size(); i ++)
			{
				std::normal_distribution<FLT_TYPE> nd(mean[i], sigma[i]);
				noise[i] = nd(engine);
			}
			return noise;
		};
		void init(T mean, T sigma)
		{
			for(auto &p: particles)
			{
				p.state = generate_noise(mean, sigma);
				p.probability = 1.0 / particles.size();
			}
		};
		void resample(T sigma)
		{
			FLT_TYPE accum = 0;
			size_t hi = 0;
			for(auto &p: particles)
			{
				accum += p.probability;
				p.accum_probability = accum;
				for(size_t i = 0; i < ie.size(); i ++)
				{
					ind_histogram[i][hi] = p.state[i];
				}
				hi ++;
			}
			for(size_t i = 0; i < ie.size(); i ++)
			{
				std::sort(ind_histogram[i].begin(), ind_histogram[i].end());
			}

			particles_dup = particles;
			FLT_TYPE pstep = accum / particles.size();
			FLT_TYPE pscan = 0;
			auto it = particles_dup.begin();
			auto it_prev = particles_dup.end();

			FLT_TYPE prob = 1.0 / particles.size();
			for(auto &p: particles)
			{
				pscan += pstep;
				it = std::lower_bound(it, particles_dup.end(), particle(pscan));
				particle p0 = *it;
				if(it == it_prev)
				{
					p.state = p0.state + generate_noise(T(), sigma);
				}
				else if(it == particles_dup.end())
				{
					p.state = it_prev->state;
				}
				else
				{
					p.state = p0.state;
				}
				it_prev = it;
				p.probability = prob;
			}
		};
		void noise(T sigma)
		{
			for(auto &p: particles)
			{
				p.state = p.state + generate_noise(T(), sigma);
			}
		};
		void predict(std::function<void(T&)> model)
		{
			for(auto &p: particles)
			{
				model(p.state);
			}
		};
		void measure(std::function<FLT_TYPE(const T&)> likelihood)
		{
			FLT_TYPE sum = 0;
			for(auto &p: particles)
			{
				p.probability *= likelihood(p.state);
				sum += p.probability;
			}
			for(auto &p: particles)
			{
				p.probability /= sum;
			}
		};
		T expectation(const FLT_TYPE pass_ratio = 1.0)
		{
			T e;
			FLT_TYPE p_sum = 0;

			if(pass_ratio < 1.0)
				std::sort(particles.rbegin(), particles.rend());
			for(auto &p: particles)
			{
				for(size_t i = 0; i < ie.size(); i ++)
				{
					e[i] += p.probability * p.state[i];
				}
				p_sum += p.probability;
				if(p_sum > pass_ratio) break;
			}
			for(size_t i = 0; i < ie.size(); i ++)
			{
				e[i] /= p_sum;
			}
			return e;
		};
		T max()
		{
			T *m = &particles[0].state;
			FLT_TYPE max_probability = particles[0].probability;
			for(auto &p: particles)
			{
				if(max_probability < p.probability)
				{
					max_probability = p.probability;
					m = &p.state;
				}
			}
			return *m;
		};
		const T get_particle(const size_t i)
		{
			return particles[i].state;
		}
		const size_t get_particle_size()
		{
			return particles.size();
		}
	private:
		class particle
		{
		public:
			particle()
			{
				probability = 0.0;
			};
			particle(FLT_TYPE prob)
			{
				accum_probability = prob;
			};
			T state;
			FLT_TYPE probability;
			FLT_TYPE accum_probability;
			const bool operator<(const particle &p2) const
			{
				return this->accum_probability < p2.accum_probability;
			};
		};
		std::vector<particle> particles;
		std::vector<particle> particles_dup;
		std::vector<std::vector<FLT_TYPE>> ind_histogram;
		std::random_device seed_gen;
		std::default_random_engine engine;
		T ie;
	};
};


